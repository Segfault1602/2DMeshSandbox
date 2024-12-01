#pragma once

#include <cstddef>

enum class FFTWindowType
{
    Rectangular,
    Hamming,
    Hann,
    Blackman
};

struct SpectrogramInfo
{
    int fft_size;
    int num_bins;
    int overlap;
    int fft_hop_size;
    int samplerate;
    int num_freqs;
};

void GetWindow(FFTWindowType type, float* window, size_t count);

void fft(const float* in, float* out, size_t count);

void fft_abs(const float* in, float* out, size_t count);