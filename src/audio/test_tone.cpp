#include "test_tone.h"

#include <cmath>
#include <numbers>

namespace
{
constexpr float k_two_pi = 2 * std::numbers::pi_v<float>;
}

TestToneGenerator::TestToneGenerator()
    : phase_increment_(k_two_pi * frequency_ / sample_rate_)
{
}

TestToneGenerator::~TestToneGenerator() = default;

void TestToneGenerator::SetSampleRate(uint32_t sample_rate)
{
    sample_rate_ = sample_rate;
    phase_increment_ = k_two_pi * frequency_ / sample_rate_;
}

void TestToneGenerator::SetFrequency(float frequency)
{
    frequency_ = frequency;
    phase_increment_ = k_two_pi * frequency_ / sample_rate_;
}

void TestToneGenerator::SetGain(float gain)
{
    gain_ = gain;
}

float TestToneGenerator::Tick()
{
    float sample = gain_ * std::sin(phase_);
    phase_ += phase_increment_;
    if (phase_ >= k_two_pi)
    {
        phase_ -= k_two_pi;
    }
    return sample;
}