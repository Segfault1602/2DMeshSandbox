#include "wave_math.h"

#include <algorithm>
#include <cmath>
#include <numbers>

float get_wave_speed(float tension, float density)
{
    return sqrt(tension / density);
}

float get_sample_distance(float wave_speed, float sample_rate)
{
    return std::numbers::sqrt2_v<float> * wave_speed / sample_rate;
}

float get_max_radius(float radius, float friction_delay, float sample_distance, float min_delay)
{
    // 1.5 is the minimum delay because the allpass delay is has a minimum of 0.5 and
    // the scattering junction between the mesh and the rimguide has an inherent delay of 1 sample
    min_delay = std::max(min_delay, 1.5f);

    return radius - ((min_delay + friction_delay) * sample_distance * 0.5f);
}

float get_fundamental_frequency(float radius, float wave_speed, float sample_rate)
{
    return (2.405f * wave_speed) / (sample_rate * radius);
}

float get_friction_coeff(float radius, float wave_speed, float decay_rate, float fund_freq)
{
    if (decay_rate > 0)
    {
        decay_rate *= -1;
    }

    if (decay_rate == 0.f)
    {
        return 0.f;
    }

    float diameter = radius * 2;
    float gain = std::pow(10.f, (decay_rate * diameter) / (20.f * wave_speed));
    const float cos_freq = std::cos(fund_freq);
    const float gain_2 = gain * gain;
    float coeff = (1 - gain_2 * cos_freq -
                   std::sqrt((std::pow(gain, 4) * (std::pow(cos_freq, 2) - 1)) + (gain_2 * (2 - 2 * cos_freq)))) /
                  (gain_2 - 1);
    return coeff;
}

float get_friction_delay(float friction_coeff, float fund_freq)
{
    return (std::atan((-friction_coeff * std::sin(fund_freq)) / (friction_coeff * std::cos(fund_freq) + 1)) /
            fund_freq);
}

std::array<size_t, 2> get_grid_size(float radius, float sample_distance, float vertical_scaler)
{
    float radius_sample = radius / sample_distance;
    size_t x = static_cast<size_t>(std::ceil(radius_sample * 2));
    size_t y = static_cast<size_t>(std::ceil(radius_sample * 2 * vertical_scaler));

    return {x, y};
}