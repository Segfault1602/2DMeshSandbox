#include "file_writer.h"
#include "gaussian.h"
#include "rectilinear_mesh.h"
#include "rimguide.h"
#include "rimguide_utils.h"
#include "wave_math.h"

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sndfile.h>
#include <string>
#include <thread>

constexpr float kSampleRate = 11025;

constexpr float kDensity = 0.262;
constexpr float kRadius = 0.05; // radius of the membrane

constexpr float kTension = 3325.f;
constexpr float kDecay = 25.f;

constexpr float kDurationSeconds = 2.0f;
constexpr size_t kOutputSize = kSampleRate * kDurationSeconds;

constexpr char kOutputFile[] = "mesh_graph_test2.wav";

std::vector<float> render(float tension, float density, float radius)
{
    float c = get_wave_speed(tension, density);
    std::cout << "Wave speed: " << c << " m/s" << std::endl;

    float sample_distance = get_sample_distance(c, kSampleRate);
    std::cout << "Sample distance: " << sample_distance << " m" << std::endl;

    float f0 = get_fundamental_frequency(radius, c, kSampleRate);
    std::cout << "Fundamental frequency: " << f0 << std::endl;
    float f0_hz = f0 * kSampleRate / (2 * M_PI);
    std::cout << "Fundamental frequency (Hz): " << f0_hz << std::endl;

    float friction_coeff = get_friction_coeff(radius, c, kDecay, f0);
    std::cout << "Friction coefficient: " << friction_coeff << std::endl;

    float friction_delay = get_friction_delay(friction_coeff, f0);
    std::cout << "Friction delay: " << friction_delay << " s" << std::endl;

    float max_radius = get_max_radius(kRadius, friction_delay, sample_distance);
    std::cout << "Max radius: " << max_radius << " m" << std::endl;

    auto grid_size = get_grid_size(max_radius, sample_distance);
    std::cout << "Grid size: " << grid_size[0] << " x " << grid_size[1] << std::endl;

    const size_t kGridX = grid_size[0];
    const size_t kGridY = grid_size[1];

    RimguideInfo info{};
    info.friction_coeff = -friction_coeff;
    info.friction_delay = friction_delay;
    info.wave_speed = c;
    info.sample_rate = kSampleRate;
    info.is_solid_boundary = true;
    info.get_rimguide_pos = std::bind(get_boundary_position, kRadius, std::placeholders::_1);

    std::vector<float> out_buffer(kOutputSize, 0.0f);
    out_buffer.reserve(kOutputSize);

    RectilinearMesh mesh_graph(kGridX, kGridY, sample_distance);
    auto mask = mesh_graph.get_mask_for_radius(max_radius);

    mesh_graph.init(mask);
    mesh_graph.init_boundary(info);

    mesh_graph.set_input(0.5f, 0.5f);
    mesh_graph.set_output(0.5f, 0.5f);
    mesh_graph.set_absorption_coeff(0.9f);

    auto impulse = std::vector<float>{1}; // raised_cosine(100, kSampleRate);

    auto start = std::chrono::high_resolution_clock::now();

    for (auto i = 0; i < kOutputSize - 1; i++)
    {
        float input = 0.f;
        if (i < impulse.size())
        {
            input = -impulse[i];
        }
        out_buffer[i] = mesh_graph.tick(input);

        // mesh_graph.print_junction_pressure();
        // std::cout << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto render_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << std::endl;
    std::cout << "Simulation done" << std::endl;
    std::cout << "Render time: " << render_time << " s" << std::endl;

    return out_buffer;
}

int main()
{
    std::cout << "mesh_graph_test" << std::endl;

    SF_INFO out_sf_info{0};
    out_sf_info.channels = 1;
    out_sf_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    out_sf_info.samplerate = static_cast<int>(kSampleRate);
    out_sf_info.frames = static_cast<sf_count_t>(kOutputSize);

    auto out_buffer = render(kTension, kDensity, kRadius);

    SNDFILE* out_file = sf_open(kOutputFile, SFM_WRITE, &out_sf_info);
    if (!out_file)
    {
        std::cerr << "Failed to open output file" << std::endl;
        return 0;
    }

    sf_writef_float(out_file, out_buffer.data(), kOutputSize);
    sf_write_sync(out_file);
    sf_close(out_file);

    return 0;
}