#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "line.h"
#include "listener.h"
#include "mesh_2d.h"
#include "mesh_manager.h"
#include "trimesh.h"
#include "vec2d.h"

#include <atomic>
#include <cstdint>
#include <memory>

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

/**
 * @brief Manages the circular mesh for rendering and simulation.
 */
class CircularMeshManager : public MeshManager
{
  public:
    /**
     * @brief Constructs a new CircularMeshManager object.
     */
    CircularMeshManager();

    /**
     * @brief Destroys the CircularMeshManager object.
     */
    ~CircularMeshManager() override;

    /**
     * @brief Draws the configuration menu.
     * @param reset_camera [out] flag to reset the camera.
     */
    void draw_config_menu(bool& reset_camera) override;

    /**
     * @brief Draws the experimental configuration menu.
     */
    void draw_experimental_config_menu() override;

    /**
     * @brief Draws the simulation menu.
     * @param reset_camera [out] flag to reset the camera.
     */
    void draw_simulation_menu(bool& reset_camera);

    /**
     * @brief Renders the mesh asynchronously.
     * @param render_time_seconds Time in seconds for rendering.
     * @param cb Callback function to be called upon render completion.
     */
    void render_async(float render_time_seconds, RenderCompleteCallback cb) override;

    /**
     * @brief Gets the progress of the rendering.
     * @return Progress as a float.
     */
    float get_progress() const override;

    /**
     * @brief Checks if rendering is in progress.
     * @return True if rendering, false otherwise.
     */
    bool is_rendering() const override;

    /**
     * @brief Gets the runtime of the render.
     * @return Render runtime in milliseconds.
     */
    float get_render_runtime() const override;

    /**
     * @brief Plots the mesh.
     */
    void plot_mesh() const override;

    /**
     * @brief Renders the mesh using OpenGL.
     * @param mvp Model-View-Projection matrix.
     */
    void render_gl_mesh(glm::mat4 mvp) const override;

    float current_fundamental_frequency() const;

  private:
    /**
     * @brief Computes the parameters for the mesh.
     */
    void compute_parameters();

    /**
     * @brief Updates the mesh object.
     */
    void update_mesh_object();

    /**
     * @brief Worker function for asynchronous rendering.
     * @param mesh Unique pointer to the mesh object.
     * @param cb Callback function to be called upon render completion.
     */
    void render_async_worker(std::unique_ptr<Mesh2D>&& mesh, RenderCompleteCallback cb);

    /**
     * @brief Updates the OpenGL mesh.
     */
    void update_gl_mesh();

    RimguideInfo get_rimguide_info() const;

    MeshType mesh_type_ = MeshType::TRIANGULAR_MESH; ///< Type of the mesh.
    std::unique_ptr<Mesh2D> mesh_{nullptr};          ///< Pointer to the mesh object.
    Listener listener_;                              ///< Listener for events.
    std::atomic_bool is_rendering_{false};           ///< Flag indicating if rendering is in progress.
    std::atomic<float> progress_{0.f};               ///< Progress of the rendering.
    std::atomic<float> render_runtime_{0.f};         ///< Runtime of the render in milliseconds.

    // Model parameters
    int32_t sample_rate_ = 11025;         ///< Sample rate for the simulation.
    float radius_ = 0.32;                 ///< Radius of the circular mesh.
    float decays_ = 25.f;                 ///< Decay factor for the simulation.
    bool use_custom_cutoff_ = false;      ///< Flag to use custom cutoff frequency.
    float cutoff_freq_ = 0.f;             ///< Custom cutoff frequency.
    float cutoff_freq_hz_ = 1000.f;       ///< Cutoff frequency in Hz.
    float density_ = 0.262;               ///< Density of the mesh material.
    int32_t tension_ = 3325.f;            ///< Tension of the mesh.
    float minimum_rimguide_delay_ = 1.5f; ///< Minimum delay for the rim guide.
    bool is_solid_boundary_ = true;       ///< Flag indicating if the boundary is solid.
    float render_time_seconds_ = 1.f;     ///< Time in seconds for rendering.
    Vec2Df input_pos_ = {0.5f, 0.5f};     ///< Input position on the mesh.
    Vec2Df output_pos_ = {0.5f, 0.5f};    ///< Output position on the mesh.

    bool is_simulation_running_ = false; ///< Flag indicating if the simulation is running.
    float vertical_scaler_ = 1.f;        ///< Vertical scaler for rendering.

    // Derived parameters
    float wave_speed_ = 0;            ///< Speed of the wave propagation.
    float sample_distance_ = 0;       ///< Distance between samples.
    float fundamental_frequency_ = 0; ///< Fundamental frequency of the mesh.
    float friction_coeff_ = 0.f;      ///< Friction coefficient.
    float friction_delay_ = 0.f;      ///< Friction delay.
    float max_radius_ = 0.f;          ///< Maximum radius of the mesh.
    Vec2Di grid_size_ = {0, 0};       ///< Size of the grid.

    // Excitation parameters
    ExcitationType excitation_type_ = ExcitationType::RAISE_COSINE; ///< Type of excitation.
    float excitation_frequency_ = 100.f;                            ///< Frequency of the excitation.
    float excitation_amplitude_ = 1.f;                              ///< Amplitude of the excitation.
    std::string excitation_filename_{""};

    ListenerType listener_type_ = ListenerType::ALL; ///< Type of listener.

    enum class TimeVaryingAllpassType
    {
        SYNC,
        PHASE_OFFSET,
        RANDOM,
        RANDOM_FREQ_AND_AMP,
    };

    // Experimental parameters
    bool use_time_varying_allpass_ = false;                              ///< Flag to use time-varying allpass filter.
    TimeVaryingAllpassType allpass_type_ = TimeVaryingAllpassType::SYNC; ///< Type of time-varying allpass filter.
    float allpass_mod_freq_ = 1.f;                                       ///< Modulation frequency for allpass filter.
    float allpass_mod_amp_ = 1.f;                                        ///< Modulation amplitude for allpass filter.
    float allpass_phase_offset_ = 0.f;                                   ///< Phase offset for allpass filter.
    float allpass_random_freq_ = 1.f;                                    ///< Random frequency for allpass filter.
    float allpass_random_mod_amp_ = 1.f; ///< Random modulation amplitude for allpass filter.

    bool clamp_center_ = false; ///< Flag to clamp the center.

    bool use_automatic_pitch_bend_ = false; ///< Flag to use automatic pitch bend.
    float pitch_bend_amount_ = 0.f;         ///< Amount of pitch bend.

    bool use_square_law_nonlinearity_ = false; ///< Flag to use square law nonlinearity.
    float nonlinear_factor_ = 0.1f;            ///< Nonlinear factor.

    bool use_nonlinear_allpass_ = false;             ///< Flag to use nonlinear allpass filter.
    float nonlinear_allpass_coeffs_[2] = {0.f, 0.f}; ///< Coefficients for nonlinear allpass filter.

    bool use_dc_blocker_ = false;     ///< Flag to use DC blocker.
    float dc_blocker_alpha_ = 0.995f; ///< Alpha value for DC blocker.

    bool use_extra_diffusion_filters_ = false;
    size_t diffusion_filter_count_ = 0;
    std::vector<float> diffusion_filter_coeffs_;

    std::unique_ptr<Line> line_ = nullptr;        ///< Pointer to the line object.
    std::unique_ptr<Line> circle_line_ = nullptr; ///< Pointer to the circular line object.
};