Noiez — Real-Time Noise Suppression

A C++ application called Noiez that performs real-time noise suppression on microphone input using the RNNoise library and PortAudio for audio I/O.
Features

    Real-time audio capture and playback using PortAudio

    Noise suppression using RNNoise denoiser

    Ring buffer implementation for smooth audio streaming

    Adjustable wet/dry mix for noise suppression effect blending

    Input and output overrun/underrun diagnostics

    Support for x86 denormal float control for improved performance

Requirements

    C++17 compatible compiler

    PortAudio installed and linked

    RNNoise library installed and linked

    SSE support on x86 systems (optional for performance)

    Standard C++ libraries: <vector>, <array>, <iostream>, etc.

Build Instructions

    Clone or download this repository.

    Ensure PortAudio and RNNoise libraries are installed on your system.

    Compile the code linking against PortAudio and RNNoise. For example (Linux):

g++ -std=c++17 -O3 main.cpp -lportaudio -lrnnoise -o noiez

Adjust compilation flags as needed for your platform.
Usage

Run the compiled executable:

./noiez

    The program captures audio from the default input device (microphone).

    Applies noise suppression in real-time.

    Outputs the processed audio to the default output device (speakers/headphones).

    Press Enter to stop the program.

How It Works

    Captures audio frames of size 480 samples at 48kHz.

    Stores audio in ring buffers for input and output.

    Processes each frame with RNNoise to reduce noise.

    Mixes the processed ("wet") and original ("dry") audio according to a wet_mix factor.

    Plays back the cleaned audio with minimal latency.

    Tracks audio input/output buffer overruns for diagnostics.

Notes

    Ensure your microphone and speakers are properly configured.

    Requires PortAudio and RNNoise installed and accessible on your system.

    Performance can be improved on x86 by enabling denormal number handling (FTZ/DAZ).
