**Voice Recognizer — ML-Based Hotword Detection and Speech-to-Text (Qt/QML + whisper.cpp)
**
**Overview**

This project demonstrates how to integrate whisper.cpp into a C++/QML application to enable ML-based hotword detection and speech-to-text functionality in an efficient and production-oriented manner. Voice Recognizer is an offline speech recognition application built with C++ and QML.
It uses ML-based Speech-to-Text via whisper.cpp to continuously listen for a predefined hotword in the background and transcribe user voice commands into text.

The application is designed to be lightweight, responsive, and optimized for continuous listening by separating hotword detection and command transcription into two different STT stages.

**How It Works
**
The application continuously captures microphone audio in the background. A lightweight Whisper model is used to perform real-time STT for hotword detection. Incoming audio is continuously transcribed and compared against a predefined hotword.

When the hotword is detected:

-Hotword listening is stopped
-The microphone is activated for 5 seconds
-User speech is recorded
-The recorded audio is then processed by a more accurate Whisper model.

The final transcription result is converted to text and displayed in the UI.

**Model Selection & Optimization
**
To balance performance and accuracy, the application uses two different Whisper models:

-ggml-tiny-q8_0.bin
Used for continuous background hotword listening due to its low latency and reduced CPU usage.

-ggml-base.en.bin
Used for voice command transcription to achieve higher transcription accuracy.

This two-stage approach allows the application to remain responsive while minimizing resource consumption during continuous listening.

UI Interaction (QML)

When a hotword is detected, the backend emits a signal to the QML-based UI, which:

Activates a microphone animation (GIF)

Indicates that voice recording is active for 5 seconds

Displays the transcribed text after processing

This ensures clear visual feedback to the user during voice interaction.

Audio Capture

Microphone input is captured using the PortAudio library.
PortAudio was chosen over QtMultimedia due to its:

Lower latency
Lightweight footprint
Better performance for continuous audio streaming

**Key Technologies
**C++
Qt / QML

whisper.cpp (ML-based STT)

PortAudio
