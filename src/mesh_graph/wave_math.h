#pragma once

#include <array>
#include <cstddef>

/**
 * Calculates the wave propagation speed in a material
 * @param tension Material tension (N/m)
 * @param density Material density (kg/m^2)
 * @return Wave speed in meters per second
 */
float get_wave_speed(float tension, float density);

/**
 * Calculates the spatial distance between samples
 * @param wave_speed Wave propagation speed (m/s)
 * @param sample_rate Temporal sampling rate (Hz)
 * @return Sample distance in meters
 */
float get_sample_distance(float wave_speed, float sample_rate);

/**
 * Calculates the maximum possible radius for junctions before the rimguides
 * @param radius Initial radius value
 * @param friction_delay Friction delay coefficient
 * @param sample_distance Distance between samples
 * @return Maximum radius
 */
float get_max_radius(float radius, float friction_delay, float sample_distance, float min_delay = 1.5f);

/**
 * Calculates the fundamental frequency of the mesh
 * @param radius Mesh radius
 * @param wave_speed Wave propagation speed
 * @param sample_rate Temporal sampling rate
 * @return Fundamental frequency in Hz
 */
float get_fundamental_frequency(float radius, float wave_speed, float sample_rate);

/**
 * Calculates the friction coefficient for decay simulation
 * @param radius Mesh radius
 * @param wave_speed Wave propagation speed
 * @param decay_rate Desired decay rate
 * @param fund_freq Fundamental frequency
 * @return Friction coefficient
 */
float get_friction_coeff(float radius, float wave_speed, float decay_rate, float fund_freq);

/**
 * Calculates the friction delay based on the coefficient
 * @param friction_coeff Friction coefficient
 * @param fund_freq Fundamental frequency
 * @return Friction delay value
 */
float get_friction_delay(float friction_coeff, float fund_freq);

std::array<size_t, 2> get_grid_size(float radius, float sample_distance, float vertical_scaler = 1);