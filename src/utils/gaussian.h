#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

inline std::vector<float> gaussian(int size)
{
    std::vector<float> kernel(size);
    constexpr float sigma = 2.5465f;

    float offset = (size - 1.f) / 2.f;
    for (int i = 0; i < size; ++i)
    {
        float x = i - offset;
        kernel[i] = std::exp(-x * x / (2.f * sigma * sigma));
    }

    return kernel;
}

inline std::vector<float> raised_cosine(float freq, float sample_rate)
{
    std::vector<float> impulse;
    float dt = 1.f / sample_rate;
    float period = 1.f / freq;
    float phase = 0.f;
    while (phase < period)
    {
        impulse.push_back(0.5f * (1.f - std::cos(2.f * M_PI * phase * freq)));
        phase += dt;
    }

    return impulse;
}