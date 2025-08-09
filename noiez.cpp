#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <portaudio.h>
#include "rnnoise.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <xmmintrin.h>
#define CAN_CONTROL_DENORMALS 1
#endif

static constexpr int SAMPLE_RATE = 48000;
static constexpr size_t RNNOISE_FRAME_SIZE = 480;
static constexpr size_t RING_CAPACITY = 8192;
static_assert((RING_CAPACITY & (RING_CAPACITY - 1)) == 0, "RING_CAPACITY must be a power of two.");

struct RingBuffer {
    std::array<float, RING_CAPACITY> buffer{};
    size_t write_pos = 0;
    size_t read_pos = 0;
    size_t count = 0;

    size_t capacity() const { return buffer.size(); }
    size_t space() const { return capacity() - count; }

    size_t write(const float* src, size_t n) {
        n = std::min(n, space());
        if (!n) return 0;
        const size_t cap = capacity();
        size_t first = std::min(n, cap - write_pos);
        std::memcpy(&buffer[write_pos], src, first * sizeof(float));
        size_t second = n - first;
        if (second) std::memcpy(&buffer[0], src + first, second * sizeof(float));
        write_pos = (write_pos + n) & (cap - 1);
        count += n;
        return n;
    }

    size_t read(float* dst, size_t n) {
        n = std::min(n, count);
        if (!n) return 0;
        const size_t cap = capacity();
        size_t first = std::min(n, cap - read_pos);
        std::memcpy(dst, &buffer[read_pos], first * sizeof(float));
        size_t second = n - first;
        if (second) std::memcpy(dst + first, &buffer[0], second * sizeof(float));
        read_pos = (read_pos + n) & (cap - 1);
        count -= n;
        return n;
    }
};

struct CallbackData {
    DenoiseState* rnnoise_state = nullptr;
    RingBuffer in_ring;
    RingBuffer out_ring;
    alignas(16) std::array<float, RNNOISE_FRAME_SIZE> temp_in_buffer{};
    alignas(16) std::array<float, RNNOISE_FRAME_SIZE> temp_out_buffer{};
    float wet_mix = 1.0f;
    uint64_t input_xruns = 0;
    uint64_t output_xruns = 0;

    CallbackData() { rnnoise_state = rnnoise_create(nullptr); }
    ~CallbackData() { if (rnnoise_state) rnnoise_destroy(rnnoise_state); }
};

static int audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo*,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    auto* data = static_cast<CallbackData*>(userData);
    const auto* in_ptr = static_cast<const float*>(input);
    auto* out_ptr = static_cast<float*>(output);

    if (statusFlags & paInputOverflow) data->input_xruns++;
    if (statusFlags & paOutputUnderflow) data->output_xruns++;

    if (in_ptr) {
        data->in_ring.write(in_ptr, frameCount);
    } else {
        static std::vector<float> silence_buffer(frameCount, 0.0f);
        if (silence_buffer.size() < frameCount) silence_buffer.assign(frameCount, 0.0f);
        data->in_ring.write(silence_buffer.data(), frameCount);
    }

    while (data->in_ring.count >= RNNOISE_FRAME_SIZE && data->out_ring.space() >= RNNOISE_FRAME_SIZE) {
        data->in_ring.read(data->temp_in_buffer.data(), RNNOISE_FRAME_SIZE);
        rnnoise_process_frame(data->rnnoise_state, data->temp_out_buffer.data(), data->temp_in_buffer.data());

        const float wet = data->wet_mix;
        const float dry = 1.0f - wet;
        if (wet < 1.0f) {
            for (size_t i = 0; i < RNNOISE_FRAME_SIZE; ++i) {
                data->temp_out_buffer[i] = (data->temp_in_buffer[i] * dry) + (data->temp_out_buffer[i] * wet);
            }
        }
        data->out_ring.write(data->temp_out_buffer.data(), RNNOISE_FRAME_SIZE);
    }

    size_t produced = data->out_ring.read(out_ptr, frameCount);
    if (produced < frameCount) {
        std::memset(out_ptr + produced, 0, (frameCount - produced) * sizeof(float));
    }
    return paContinue;
}

static bool checkPaError(PaError err, const std::string& message) {
    if (err != paNoError) {
        std::cerr << "PortAudio Error: " << message << " (" 
                  << Pa_GetErrorText(err) << ")" << std::endl;
        return false;
    }
    return true;
}

int main() {
#ifdef CAN_CONTROL_DENORMALS
    unsigned int csr = _mm_getcsr();
    _mm_setcsr(csr | 0x8040);
    std::cout << "✅ FTZ/DAZ flags set for performance." << std::endl;
#endif

    CallbackData data;
    if (!data.rnnoise_state) {
        std::cerr << "Error: Could not create RNNoise state." << std::endl;
        return 1;
    }
    data.wet_mix = 1.0f;

    if (!checkPaError(Pa_Initialize(), "Initialization failed")) return 1;

    PaStream* stream;
    if (!checkPaError(Pa_OpenDefaultStream(&stream, 1, 1, paFloat32, SAMPLE_RATE,
                                           paFramesPerBufferUnspecified, audioCallback, &data),
                      "Stream opening failed")) {
        Pa_Terminate();
        return 1;
    }

    if (!checkPaError(Pa_StartStream(stream), "Starting stream failed")) {
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    std::cout << "🚀 Noise suppression is active (Wet Mix: " << data.wet_mix << "). Press Enter to quit..." << std::endl;
    std::cin.get();

    checkPaError(Pa_StopStream(stream), "Stopping stream failed");
    checkPaError(Pa_CloseStream(stream), "Closing stream failed");
    Pa_Terminate();

    std::cout << "Program terminated cleanly." << std::endl;
    std::cout << "📊 Final Diagnostics:" << std::endl;
    std::cout << "  Input over/underruns (xruns): " << data.input_xruns << std::endl;
    std::cout << "  Output over/underruns (xruns): " << data.output_xruns << std::endl;

    return 0;
}
