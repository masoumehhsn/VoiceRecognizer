#ifndef STTHANDLER_H
#define STTHANDLER_H

#include <QObject>
#include <QAudioSource>
#include <QAudioFormat>
#include <QIODevice>
#include <QThread>
#include <QTimer>
#include <vector>
#include <mutex>
#include <whisper.h>
#include <portaudio.h>

class STTHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString command READ command WRITE setCommand NOTIFY commandChanged)

public:
    explicit STTHandler();
    ~STTHandler() {
        if (is_full_recording_active_) {
            StopFullRecording();
        }
        if (is_listening_for_hotword_) {
            StopHotwordMonitor();
        }
        Pa_Terminate();
        if (ctx_) {
            whisper_free(ctx_);
            ctx_ = nullptr;
        }
    }
    QString command() const { return command_; }

    void setCommand(const QString &s) {
        if (command_ == s) return;
        command_ = s;
        emit commandChanged();
    }
    static int recordCallback(const void *input_buffer, void *,
                              unsigned long frames_per_buffer,
                              const PaStreamCallbackTimeInfo *,
                              PaStreamCallbackFlags, void *user_data) {
        if (!input_buffer)
            return paContinue;

        auto *window = static_cast<STTHandler*>(user_data);
        const int16_t *input = static_cast<const int16_t *>(input_buffer);

        std::lock_guard<std::mutex> lock(window->buffer_mutex_);
        for (unsigned long i = 0; i < frames_per_buffer; ++i)
            window->audio_buffer_.push_back(input[i] / 32768.0f);

        return window->stop_flag_ ? paComplete : paContinue;
    }
    static int fullRecordCallback(const void *input_buffer, void *output_buffer,
                                  unsigned long framesPerBuffer,
                                  const PaStreamCallbackTimeInfo *time_info,
                                  PaStreamCallbackFlags status_flags,
                                  void *user_data) {
        STTHandler *hotword_listener = (STTHandler *)user_data;
        const int16_t *in = (const int16_t *)input_buffer;

        {
            std::lock_guard<std::mutex> lock(hotword_listener->buffer_mutex_);
            for (unsigned long i = 0; i < framesPerBuffer; i++) {
                hotword_listener->audio_buffer_.push_back((float)in[i] / 32768.0f);
            }
        }

        // Check the stop flag *in the main window*
        if (hotword_listener->is_full_recording_active_) {
            return paContinue;
        } else {
            return paComplete;  // Stop recording when button is pressed
        }
    }
    void StopHotwordMonitor();
    void HotwordMonitorLoop();
    void StartFullRecording();
    void StopFullRecording();

private slots:
    void MicrophoneBtnClicked();
    void RunWhisper();
    void SendCommand();
    void StartHotwordMonitor();

signals:
    void startFullRecordingSignal();
    void stopFullRecordingSignal();
    void ProcessUserCommandSignal(
        const QString &command);
    void TextIsReady(const QString& command);
    void PermissionAccepted();
    void SetInfoText(const QString &action);
    void commandChanged();

private:
    whisper_context *ctx_ = nullptr;

    PaStream *stream_ = nullptr;
    std::vector<float> audio_buffer_;
    std::mutex buffer_mutex_;
    bool stop_flag_ = false;

    bool is_listening_for_hotword_ =
        true;  // True while the HotwordMonitorLoop is running
    bool is_full_recording_active_ =
        false;  // True when button or hotword starts a long recording

    PaStream *full_record_stream_ = nullptr;  // Stream for the long recording
    PaStream *hotword_monitor_stream_ =
        nullptr;  // Stream for the continuous hotword monitor

    std::thread hotword_thread_;
    const int hotword_samples_ = 5 * 16000;

    std::atomic<bool> is_from_hotword_{false};
    std::atomic<bool> is_recording_{false};

    bool send_btn_clicked_ = false;
    QString command_ = "";
    QString hotword_trigger_ = "HiComputer";

};


#endif // STTHANDLER_H
