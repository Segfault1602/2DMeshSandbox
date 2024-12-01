#pragma once

/**
 * @file listener.h
 * @brief Defines classes and types for acoustic listening points in a 2D mesh
 */

#include "vec3d.h"
#include <DelayA.h>
#include <cstddef>
#include <vector>

#include "junction.h"

class Mesh2D;

/**
 * @brief Type of acoustic listener in the mesh
 */
enum class ListenerType
{
    ALL,      ///< Listens to all mesh elements
    BOUNDARY, ///< Listens only to boundary elements
    POINT,    ///< Single point listener
    ZONE,     ///< Listens within a specified radius
};

/**
 * @brief Contains configuration parameters for a listener
 */
struct ListenerInfo
{
    Vec3Df position;   ///< Position of the listener in 3D space
    size_t samplerate; ///< Sampling rate in Hz
    ListenerType type; ///< Type of the listener
    float radius;      ///< Listening radius (for ZONE type)
};

/**
 * @brief Implements an acoustic listener in a 2D mesh
 */
class Listener
{
  public:
    /**
     * @brief Default constructor
     */
    Listener();

    /**
     * @brief Initializes the listener with mesh and configuration
     * @param mesh Reference to the 2D mesh
     * @param info Listener configuration parameters
     */
    void init(const Mesh2D& mesh, const ListenerInfo& info);

    /**
     * @brief Sets the gain factor for the listener
     * @param gain Gain value to apply
     */
    void set_gain(float gain);

    /**
     * @brief Processes one time step and returns the accumulated sound
     * @return The calculated sound value for current time step
     */
    float tick();

  private:
    const Mesh2D* mesh_;              ///< Pointer to the associated mesh
    std::vector<stk::DelayA> delays_; ///< Delay lines for acoustic simulation
    std::vector<float> loss_factors_; ///< Attenuation factors
    Vec3Df pos_{};                    ///< Listener position
    float gain_ = 1.f;                ///< Gain factor
    ListenerType type_;               ///< Type of the listener

    const Junction* point_source_{nullptr};
};