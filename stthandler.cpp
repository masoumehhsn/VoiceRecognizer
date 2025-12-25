#include "stthandler.h"
#include <QDebug>
#include <QAudioDevice>
#include <QMediaDevices>
#include <chrono>

STTHandler::STTHandler(){
    connect(this, &STTHandler::startFullRecordingSignal, this,
            &STTHandler::StartFullRecording);
    connect(this, &STTHandler::stopFullRecordingSignal, this,
            &STTHandler::StopFullRecording);
    StartHotwordMonitor();
}

void STTHandler::StopHotwordMonitor()
{
    if (!is_listening_for_hotword_) {
        qDebug() << "Hotword monitor is not running.";
        return;
    }
    stop_flag_ = true;
    if (hotword_monitor_stream_) {
        Pa_StopStream(hotword_monitor_stream_);
        Pa_CloseStream(hotword_monitor_stream_);
        hotword_monitor_stream_ = nullptr;
        qDebug() << "PortAudio hotword stream closed.";
    }

    if (hotword_thread_.joinable()) {
        hotword_thread_.join();
        qDebug() << "Hotword monitoring thread joined.";
    }

    is_listening_for_hotword_ = false;
    stop_flag_ = false;  // Reset stopFlag for future use
}

void STTHandler::HotwordMonitorLoop()
{
    if (!ctx_) {
        ctx_ = whisper_init_from_file("../../model/ggml-tiny-q8_0.bin");
        if (!ctx_) {
            qDebug() << "Failed to load Whisper model.";
            // Consider setting a flag or emitting a signal to handle this fatal
            // error
            stop_flag_ = true;
            return;
        }
    }

    whisper_full_params params =
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    params.language = "en";
    params.n_threads = 1;      // Low thread count for minimal CPU impact
    params.no_context = true;  // Crucial: Treats each buffer as a new utterance
    params.no_timestamps = true;  // Timestamps are not needed for KWS

    params.suppress_blank = true;
    params.prompt_n_tokens = 0;  // Ensure no previous prompt is used



    const float KWS_THRESHOLD_NO_SPEECH_PROB = 0.10f;

    while (!stop_flag_) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(2000));  // Sleep to prevent high CPU usage

        std::vector<float> audioCopy;

        // Lock and copy the latest audio buffer (sliding window)
        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            if (audio_buffer_.size() < hotword_samples_) {
                continue;
            }
            audioCopy.assign(audio_buffer_.end() - hotword_samples_,
                             audio_buffer_.end());
        }

        if (whisper_full(ctx_, params, audioCopy.data(), audioCopy.size()) !=
            0) {
            qDebug() << "Whisper hotword decoding error.";
            continue;
        }
        if (whisper_full_n_segments(ctx_) > 0) {
            QString text = QString::fromUtf8(whisper_full_get_segment_text(ctx_, 0));

            float no_speech_prob =
                whisper_full_get_segment_no_speech_prob(ctx_, 0);
            QString filtered;
            for (QChar c : text)
                if (c.isLetter())
                    filtered.append(c);
            text = filtered;

            //Check 1: Does the transcription match the hotword?
            qDebug() << text;
            bool text_matches =
                hotword_trigger_.compare((text), Qt::CaseInsensitive) == 0;
            qDebug() << "text"<< text;
            bool speech_is_confident =
                (no_speech_prob < KWS_THRESHOLD_NO_SPEECH_PROB);

            if (text_matches && speech_is_confident) {
                qDebug() << "HOTWORD DETECTED: " << text
                         << " (No-Speech Prob: " << no_speech_prob << ")";
                emit SetInfoText("Hotword detected! activating microphone...");
                {
                    std::lock_guard<std::mutex> lock(buffer_mutex_);
                    audio_buffer_.clear();

                }
                emit startFullRecordingSignal();
                is_from_hotword_.store(true);

                //std::this_thread::sleep_for(std::chrono::seconds(5));
            } else {
                qDebug() << "Filter: '" << text
                         << "' (No-Speech Prob: " << no_speech_prob << ").";
            }
        }
    }

    // --- 5. Cleanup ---
    whisper_free(ctx_);
    ctx_ = nullptr;
    qDebug() << "Whisper monitoring thread terminated.";

}

void STTHandler::StartFullRecording()
{
    qDebug() << "start recording";

    if (is_listening_for_hotword_) {
        StopHotwordMonitor();
        QTimer::singleShot(6000, this,
                           [this]() { emit stopFullRecordingSignal(); });
    }
    is_recording_ = true;
    audio_buffer_.clear();
    is_full_recording_active_ = true;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qDebug() << "PortAudio initialization error in full record:"
                 << Pa_GetErrorText(err);
        return;
    }

    Pa_OpenDefaultStream(&full_record_stream_, 1, 0, paInt16, 16000, 512,
                         fullRecordCallback, this);
    Pa_StartStream(full_record_stream_);
    qDebug()
        << "Full Recording started, waiting for Stop button or 5s timeout...";
}

void STTHandler::StopFullRecording() {
    is_recording_ = false;

    qDebug() << "stop recording";
    if (!is_full_recording_active_)
        return;

    // 1. Stop PortAudio Stream
    is_full_recording_active_ = false;
    Pa_StopStream(full_record_stream_);
    Pa_CloseStream(full_record_stream_);

    qDebug() << "Full Recording stopped. Processing command...";

    RunWhisper();
}

void STTHandler::MicrophoneBtnClicked()
{
    if (is_recording_) {
        StopFullRecording();
    } else {
        StartFullRecording();

    }

}

void STTHandler::RunWhisper()
{
    if (!ctx_) {
        ctx_ = whisper_init_from_file("../../model/ggml-tiny-q8_0.bin");
        if (!ctx_) {
            qDebug() << "Failed to load Whisper model.";
            return;
        }
    }

    whisper_full_params params =
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.language = "en";
    params.translate = false;

    std::vector<float> audioCopy;
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        audioCopy = audio_buffer_;
    }

    qDebug() << "Running Whisper on" << audioCopy.size() << "samples...";
    if (whisper_full(ctx_, params, audioCopy.data(), audioCopy.size()) != 0) {
        qDebug() << "Whisper failed.";
        return;
    }
    // output_edit_->clear();
    int n = whisper_full_n_segments(ctx_);
    QString text;
    for (int i = 0; i < n; ++i) {
        text.append(whisper_full_get_segment_text(ctx_, i));
    }
    setCommand(text);
    // output_edit_->setText(text);
    if (text != "") {
        // info_lbl_->setText("Command is ready to send");
    }
    else {
        if(!is_listening_for_hotword_) StartHotwordMonitor();
    }

}

void STTHandler::SendCommand()
{
    //implement your code to send the command
}

void STTHandler::StartHotwordMonitor()
{
    if (Pa_Initialize() != paNoError) {
        qDebug() << "Failed to init PortAudio";
        return;
    }

    Pa_OpenDefaultStream(&hotword_monitor_stream_, 1, 0, paInt16, 16000, 512,
                         recordCallback,  // Use the SLIDING WINDOW callback
                         this);
    Pa_StartStream(hotword_monitor_stream_);
    qDebug() << "Hotword Monitor started...";

    is_listening_for_hotword_ = true;
    hotword_thread_ = std::thread(&STTHandler::HotwordMonitorLoop, this);

}
