#pragma once

#include <functional>
#include <glm/glm.hpp>

#include "listener.h"
#include "mesh_2d.h"

using RenderCompleteCallback = std::function<void()>;

/**
 * @brief Enum class for mesh type.
 */
enum class MeshType
{
    TRIANGULAR_MESH,
    RECTILINEAR_MESH
};

/**
 * @brief Enum class for excitation type.
 */
enum class ExcitationType
{
    RAISE_COSINE,
    DIRAC,
    FILE,
};

class MeshManager
{
  public:
    MeshManager() = default;
    virtual ~MeshManager() = default;

    virtual void draw_config_menu(bool& reset_camera) = 0;
    virtual void draw_experimental_config_menu() = 0;
    virtual void render_async(float render_time_seconds, RenderCompleteCallback cb) = 0;

    virtual float get_progress() const;
    virtual bool is_rendering() const;
    virtual float get_render_runtime() const;

    virtual void plot_mesh() const = 0;

    virtual void render_gl_mesh(glm::mat4 mvp) const = 0;

  protected:
    virtual void render_async_worker(std::unique_ptr<Mesh2D>&& mesh, RenderCompleteCallback cb);

    int32_t sample_rate_ = 11025; ///< Sample rate for the simulation.

    ExcitationType excitation_type_ = ExcitationType::RAISE_COSINE; ///< Type of excitation.
    float excitation_frequency_ = 100.f;                            ///< Frequency of the excitation.
    float excitation_amplitude_ = 1.f;                              ///< Amplitude of the excitation.
    std::string excitation_filename_{""};
    ListenerType listener_type_ = ListenerType::ALL; ///< Type of listener.

    float render_time_seconds_ = 1.f; ///< Time in seconds for rendering.

    bool use_dc_blocker_ = false;     ///< Flag to use DC blocker.
    float dc_blocker_alpha_ = 0.995f; ///< Alpha value for DC blocker.

    std::atomic_bool is_rendering_{false};   ///< Flag indicating if rendering is in progress.
    std::atomic<float> progress_{0.f};       ///< Progress of the rendering.
    std::atomic<float> render_runtime_{0.f}; ///< Runtime of the render in milliseconds.
};