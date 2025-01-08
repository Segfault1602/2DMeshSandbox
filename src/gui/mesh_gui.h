#pragma once

#include "audio.h"
#include "glm/ext/matrix_float4x4.hpp"

#include <glm/glm.hpp>

enum class MeshShape : int
{
    Circle = 0,
    Rectangle = 1,
};

/**
 * @class AudioManager
 * @brief Manages audio playback and processing.
 */
class AudioManager;

/**
 * @brief Initializes the Mesh GUI.
 * @note This function should be called once before any other Mesh GUI functions.
 */
void init_mesh_gui();

/**
 * @brief Changes the mesh shape.
 * @param shape The new mesh shape.
 */
void change_mesh_shape(MeshShape shape);

/**
 * @brief Draws the audio player interface.
 * @param audio_manager Pointer to the AudioManager instance.
 */
void draw_audio_player(AudioManager* audio_manager);

/**
 * @brief Draws the mesh configuration interface.
 * @param reset_camera Reference to a boolean that indicates whether to reset the camera.
 */
void draw_mesh_config(bool& reset_camera);

/**
 * @brief Draws the experimental configuration menu.
 */
void draw_experimental_config_menu();

/**
 * @brief Draws the simulation menu.
 */
void draw_simulation_menu();

/**
 * @brief Draws the mesh simulation interface.
 */
void draw_mesh_simulation();

/**
 * @brief Draws the spectrogram interface.
 */
void draw_spectrogram();

/**
 * @brief Draws the spectrum interface.
 */
void draw_spectrum();

/**
 * @brief Draws the waveform interface.
 */
void draw_waveform();

/**
 * @brief Draws the mesh shape interface.
 */
void draw_mesh_shape();

/**
 * @brief Renders the GL mesh.
 * @param mvp The model-view-projection matrix.
 */
void render_gl_mesh(glm::mat4 mvp);
