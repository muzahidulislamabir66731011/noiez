# Noiez — Real-Time Noise Suppression

Noiez is a C++ program that applies real-time noise suppression to your microphone input using the RNNoise library and PortAudio for audio input/output.

---

## Features

- Real-time noise suppression on microphone audio  
- Uses RNNoise for high-quality denoising  
- Smooth audio streaming with ring buffers  
- Tracks input/output overruns for diagnostics  
- Supports performance optimizations on x86 CPUs  

---

## Requirements

- C++17 compatible compiler  
- [PortAudio](http://www.portaudio.com/) installed  
- [RNNoise](https://github.com/xiph/rnnoise) library installed  
- SSE support on x86 (optional for speedup)  

---

## Build Instructions

Make sure PortAudio and RNNoise are installed on your system, then compile with:

```bash
g++ -std=c++17 -O3 main.cpp -lportaudio -lrnnoise -o noiez
```
Adjust compiler flags and paths according to your platform.
Usage

Run the program with:
```bash
./noiez
```
    It will capture audio from your default microphone, reduce noise in real-time, and play it back.

    Press Enter to stop the program.

Notes

    Make sure your microphone and speakers are properly set up.

    Requires an active audio input and output device.

    Performance improves on CPUs supporting flush-to-zero and denormals-are-zero flags.
---
License

This project is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License (CC BY-NC 4.0). You can use, share, and modify this project for non-commercial purposes only. See the LICENSE file for details.

---
Acknowledgments

    RNNoise by Jean-Marc Valin

    PortAudio open-source audio library

