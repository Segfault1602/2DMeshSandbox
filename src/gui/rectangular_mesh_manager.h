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
 * @brief Manages the rectangular mesh for rendering and simulation.
 */
class RectangularMeshManager : public MeshManager
{
  public:
    /**
     * @brief Constructs a new RectangularMeshManager object.
     */
    RectangularMeshManager();

    /**
     * @brief Destroys the RectangularMeshManager object.
     */
    ~RectangularMeshManager() override;

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
     * @brief Updates the OpenGL mesh.
     */
    void update_gl_mesh();

    RimguideInfo get_rimguide_info() const;

    MeshType mesh_type_ = MeshType::RECTILINEAR_MESH; ///< Type of the mesh.
    std::unique_ptr<Mesh2D> mesh_{nullptr};           ///< Pointer to the mesh object.

    // Model parameters
    float length_ = 0.64;                 ///< Length of the rectangular mesh.
    float width_ = 0.64;                  ///< Width of the rectangular mesh.
    float filter_pole_ = 0.6f;            ///< Pole for the filter.
    float density_ = 0.262;               ///< Density of the mesh material.
    int32_t tension_ = 3325.f;            ///< Tension of the mesh.
    float minimum_rimguide_delay_ = 1.5f; ///< Minimum delay for the rim guide.
    bool is_solid_boundary_ = true;       ///< Flag indicating if the boundary is solid.
    Vec2Df input_pos_ = {0.5f, 0.5f};     ///< Input position on the mesh.
    float input_radius_ = 1.f;            ///< Input radius on the mesh.
    Vec2Df output_pos_ = {0.5f, 0.5f};    ///< Output position on the mesh.

    bool is_simulation_running_ = false; ///< Flag indicating if the simulation is running.
    float vertical_scaler_ = 1.f;        ///< Vertical scaler for rendering.

    // Derived parameters
    float wave_speed_ = 0;            ///< Speed of the wave propagation.
    float sample_distance_ = 0;       ///< Distance between samples.
    float fundamental_frequency_ = 0; ///< Fundamental frequency of the mesh.
    float friction_coeff_ = 0.f;      ///< Friction coefficient.
    float friction_delay_ = 0.f;      ///< Friction delay.
    float max_length_ = 0.0f;         ///< Maximum length of the rectangular mesh.
    float max_width_ = 0.0f;          ///< Maximum width of the rectangular mesh.
    Vec2Di grid_size_ = {0, 0};       ///< Size of the grid.

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

    bool use_extra_diffusion_filters_ = false;
    size_t diffusion_filter_count_ = 0;
    std::vector<float> diffusion_filter_coeffs_;

    std::unique_ptr<Line> line_ = nullptr;          ///< Pointer to the line object.
    std::unique_ptr<Line> boundary_line_ = nullptr; ///< Pointer to the rectangular line object.
};