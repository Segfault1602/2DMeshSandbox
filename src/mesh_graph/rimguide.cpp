#include "rimguide.h"

#include "Generator.h"
#include "junction.h"

#include <BiQuad.h>
#include <DelayA.h>
#include <OnePole.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <utility>

namespace
{

constexpr float kPitchBendScaler = 100.f;

} // namespace

Vec2Df get_boundary_position(float radius, const Vec2Df& junction_pos)
{
    float dist_2_center = get_distance({0, 0}, junction_pos);
    float x_unit = junction_pos.x / dist_2_center;
    float y_unit = junction_pos.y / dist_2_center;
    return {x_unit * radius, y_unit * radius};
}

Rimguide::Rimguide()
    : junction_(nullptr)
    , delay_(0)
    , filter_(0)
    , pos_{0, 0}
    , in_(0)
    , out_(0)
    , phase_reversal_(-1.f)
    , use_automatic_pitch_bend_(false)
    , pitch_bend_amount_(0)
    , use_square_law_nonlinearity_(true)
    , nonlinear_factor_(0.5f)
    , use_nonlinear_allpass_(false)
    , nonlinear_allpass_(0.f, 0.f)
    , modulator_(nullptr)
    , mod_amp_(0)
{
}

void Rimguide::clear()
{
    delay_line_.clear();
    filter_.clear();
    in_ = 0;
    out_ = 0;
}

void Rimguide::init(const RimguideInfo& info, Junction* junction)
{
    junction_ = junction;

    pos_ = info.get_rimguide_pos(junction->get_pos());

    float distance_2_junction = get_distance(pos_, junction_->get_pos());
    float sample_distance = info.sample_rate / info.wave_speed;
    delay_ = distance_2_junction * sample_distance;

    // The delay is doubled to account for the round trip
    // 1 is subtracted to account for the delay built-in the junction
    delay_ = delay_ * 2 - 1 - info.friction_delay;

    const uint32_t max_delay = pow(2, ceil(log2(delay_ + 1)));
    delay_line_.setMaximumDelay(max_delay);
    delay_line_.setDelay(delay_);

    filter_.setPole(info.friction_coeff);

    phase_reversal_ = info.is_solid_boundary ? -1.f : 1.f;

    float T = 1.f / info.sample_rate;
    float t0 = 1.f / info.fundamental_frequency;
    float a = std::exp(-T / t0);
    float b = std::pow((1.f - a), 2);
    float a1 = -2.f * a;
    float a2 = std::pow(a, 2);

    env_follower_.setCoefficients(b, 0, 0, a1, a2);

    use_automatic_pitch_bend_ = info.use_automatic_pitch_bend;
    pitch_bend_amount_ = info.pitch_bend_amount;

    use_square_law_nonlinearity_ = info.use_square_law_nonlinearity;
    nonlinear_factor_ = info.nonlinear_factor;

    use_nonlinear_allpass_ = info.use_nonlinear_allpass;
    nonlinear_allpass_.setA(info.nonlinear_allpass_coeffs[0], info.nonlinear_allpass_coeffs[1]);

    if (info.use_extra_diffusion_filters)
    {
        for (float coeff : info.diffusion_coeffs)
        {
            stk::PoleZero allpass;
            allpass.setAllpass(coeff);
            diffusion_filters_.emplace_back(allpass);
        }
    }
}

void Rimguide::init_center()
{
    junction_ = nullptr;
    pos_ = {0, 0};
    delay_ = 0;
    delay_line_.clear();
    filter_.clear();
    in_ = 0;
    out_ = 0;
    phase_reversal_ = 1.f;

    delay_ = 2.5;
    delay_line_.setMaximumDelay(8);
    delay_line_.setDelay(delay_);
    filter_.setPole(0.0f);
}

void Rimguide::process_scatter(float input)
{
    in_ = input;
}

void Rimguide::process_delay()
{
    if (modulator_)
    {
        float new_delay = delay_ + (modulator_->tick() * mod_amp_);
        new_delay = std::max(0.5f, new_delay);
        delay_line_.setDelay(new_delay);
    }
    if (use_automatic_pitch_bend_)
    {
        float env = env_follower_.tick(abs(in_));
        float new_delay = delay_ + (env * kPitchBendScaler * pitch_bend_amount_);
        new_delay = std::max(0.5f, new_delay);
        delay_line_.setDelay(new_delay);
    }

    if (use_square_law_nonlinearity_)
    {
        in_ = (in_ * in_) * nonlinear_factor_ + in_ * (1 - nonlinear_factor_);
    }
    if (use_nonlinear_allpass_)
    {
        in_ = nonlinear_allpass_.process(in_);
    }
    if (!diffusion_filters_.empty())
    {
        for (auto& filter : diffusion_filters_)
        {
            in_ = filter.tick(in_);
        }
    }

    out_ = delay_line_.tick(filter_.tick(in_ * phase_reversal_));
}

float Rimguide::last_in() const
{
    return in_;
}

float Rimguide::last_out() const
{
    return out_;
}

Vec2Df Rimguide::get_pos() const
{
    return pos_;
}

void Rimguide::set_modulator(std::unique_ptr<stk::Generator> modulator, float mod_amp)
{
    modulator_ = std::move(modulator);
    mod_amp_ = mod_amp;
}