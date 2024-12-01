#pragma once

#include "allpass.h"
#include "vec2d.h"

#include <BiQuad.h>
#include <DelayA.h>
#include <Generator.h>
#include <Noise.h>
#include <OnePole.h>
#include <PoleZero.h>
#include <memory>

class Junction;

/// @brief Configuration parameters for a rim guide waveguide
struct RimguideInfo
{
    float radius;                        ///< Radius of the rim guide
    float friction_coeff;                ///< Friction coefficient for damping
    float friction_delay;                ///< Delay time for friction effects
    float wave_speed;                    ///< Speed of wave propagation
    float sample_rate;                   ///< Audio sample rate in Hz
    bool is_solid_boundary;              ///< Whether this is a solid boundary
    float fundamental_frequency;         ///< Base frequency of the waveguide
    bool use_automatic_pitch_bend;       ///< Enable automatic pitch bending
    float pitch_bend_amount;             ///< Amount of pitch bend to apply
    bool use_square_law_nonlinearity;    ///< Enable square law nonlinearity
    float nonlinear_factor;              ///< Factor for nonlinear processing
    bool use_nonlinear_allpass;          ///< Enable nonlinear allpass filter
    float nonlinear_allpass_coeffs[2];   ///< Coefficients for nonlinear allpass
    bool use_extra_diffusion_filters;    ///< Enable extra diffusion filters
    std::vector<float> diffusion_coeffs; ///< Coefficients for diffusion filters
};

/// @brief Implements a waveguide model for rim excitation simulation
class Rimguide
{
  public:
    /// @brief Default constructor
    Rimguide();

    /// @brief Initialize the rimguide with configuration parameters
    /// @param info Configuration parameters
    /// @param junction Connected junction point
    void init(const RimguideInfo& info, Junction* junction);

    /// @brief Initialize center position
    void init_center();

    /// @brief Clear internal state
    void clear();

    /// @brief Process scatter step of the waveguide
    /// @param input Input sample to process
    void process_scatter(float input);

    /// @brief Process delay step of the waveguide
    void process_delay();

    /// @brief Get the last input sample
    /// @return Last input sample value
    float last_in() const;

    /// @brief Get the last output sample
    /// @return Last output sample value
    float last_out() const;

    /// @brief Get current position
    /// @return 2D position vector
    Vec2Df get_pos() const;

    /// @brief Set modulation generator
    /// @param modulator Unique pointer to generator
    /// @param mod_amp Modulation amplitude
    void set_modulator(std::unique_ptr<stk::Generator> modulator, float mod_amp);

  private:
    Junction* junction_;
    float delay_;
    stk::DelayA delay_line_;
    stk::OnePole filter_;
    Vec2Df pos_;

    float in_;
    float out_;
    float phase_reversal_;

    bool use_automatic_pitch_bend_;
    float pitch_bend_amount_;

    bool use_square_law_nonlinearity_;
    float nonlinear_factor_;
    bool use_nonlinear_allpass_;
    NonLinearAllpass nonlinear_allpass_;
    std::vector<stk::PoleZero> diffusion_filters_;

    std::unique_ptr<stk::Generator> modulator_;
    float mod_amp_;

    stk::BiQuad env_follower_;
    stk::Noise noise_;
};