#pragma once

#include <cstdint>

class TestToneGenerator
{
  public:
    TestToneGenerator();
    ~TestToneGenerator();

    void SetSampleRate(uint32_t sample_rate);
    void SetFrequency(float frequency);
    void SetGain(float gain);

    float Tick();

  private:
    uint32_t sample_rate_ = 48000;
    float frequency_ = 220.f;
    float gain_ = 0.1f;
    float phase_ = 0.f;
    float phase_increment_ = 0.f;
};