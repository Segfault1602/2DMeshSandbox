#include "file_writer.h"
#include "gaussian.h"
#include "listener.h"
#include "rimguide.h"
#include "trimesh.h"
#include "wave_math.h"

#include <BiQuad.h>
#include <chrono>
#include <iostream>
#include <numbers>
#include <sndfile.h>

constexpr float kSampleRate = 12000;

constexpr float kDensity = 0.262;
constexpr float kRadius = 0.32; // radius of the membrane

constexpr float kTension = 3325.f;
constexpr float kDecay = 25.f;

constexpr float kDurationSeconds = 1.0f;
constexpr size_t kOutputSize = kSampleRate * kDurationSeconds;
constexpr const char kOutputFile[] = "trimesh_nl.wav";

constexpr Vec3Df kListenerPos{0.0f, 0.0f, 0.8f};

int main()
{
    float c = get_wave_speed(kTension, kDensity);
    std::cout << "Wave speed: " << c << " m/s" << std::endl;

    float sample_distance = get_sample_distance(c, kSampleRate);
    std::cout << "Sample distance: " << sample_distance << " m" << std::endl;

    float f0 = get_fundamental_frequency(kRadius, c, kSampleRate);
    std::cout << "Fundamental frequency: " << f0 << std::endl;
    float f0_hz = f0 * kSampleRate / (2 * M_PI);
    std::cout << "Fundamental frequency (Hz): " << f0_hz << std::endl;

    float friction_coeff = get_friction_coeff(kRadius, c, kDecay, f0);
    std::cout << "Friction coefficient: " << friction_coeff << std::endl;

    float friction_delay = get_friction_delay(friction_coeff, f0);
    std::cout << "Friction delay: " << friction_delay << " s" << std::endl;

    float max_radius = get_max_radius(kRadius, friction_delay, sample_distance);
    std::cout << "Max radius: " << max_radius << " m" << std::endl;

    auto grid_size = get_grid_size(max_radius, sample_distance, 2.f / std::numbers::sqrt3_v<float>);
    std::cout << "Grid size: " << grid_size[0] << " x " << grid_size[1] << std::endl;

    const size_t kGridX = grid_size[0];
    const size_t kGridY = grid_size[1];

    RimguideInfo info{};
    info.friction_coeff = -friction_coeff;
    info.friction_delay = friction_delay;
    info.wave_speed = c;
    info.sample_rate = kSampleRate;
    info.is_solid_boundary = true;
    info.fundamental_frequency = f0_hz;

    info.use_nonlinear_allpass = false;
    info.nonlinear_allpass_coeffs[0] = 0.5f;
    info.nonlinear_allpass_coeffs[1] = 0.1f;
    info.get_rimguide_pos = std::bind(get_boundary_position, kRadius, std::placeholders::_1);

    TriMesh mesh(kGridX, kGridY, sample_distance);
    auto mask = mesh.get_mask_for_radius(max_radius);

    mesh.init(mask);
    mesh.init_boundary(info);

    mesh.set_input(0.25, 0.5);
    mesh.set_output(0.5, 0.5);

    float T = 1.f / kSampleRate;
    float t0 = 1.f / f0_hz;
    float a1 = exp(-T / t0);
    float b = pow((1.f - a1), 2);

    stk::BiQuad env_follower;
    env_follower.setCoefficients(b, 0, 0, -2.f * a1, pow(a1, 2));

    ListenerInfo listener_info{};
    listener_info.position = kListenerPos;
    listener_info.samplerate = mesh.get_samplerate();
    listener_info.type = ListenerType::ALL;

    Listener listen;
    listen.init(mesh, listener_info);
    listen.set_gain(0.2f);

    std::vector<float> out_buffer(kOutputSize, 0.0f);
    out_buffer.reserve(kOutputSize);

#if 0
    constexpr const char kImpulseFile[] = "roll_fast.wav";
    SF_INFO sf_info{0};
    SNDFILE* file = sf_open(kImpulseFile, SFM_READ, &sf_info);
    if (!file)
    {
        std::cerr << "Failed to open impulse file" << std::endl;
        return 0;
    }

    std::vector<float> impulse(sf_info.frames);
    sf_readf_float(file, impulse.data(), sf_info.frames);
    sf_close(file);
#endif

    auto start = std::chrono::high_resolution_clock::now();

    auto impulse = raised_cosine(100, kSampleRate);

    float progress = 0.f;

    std::cout << std::endl;

    for (auto i = 0; i < kOutputSize - 1; i++)
    {
        float input = 0.f;
        if (i < impulse.size())
        {
            input = -impulse[i];
        }
        mesh.tick(input);
        out_buffer[i] = listen.tick();

        float new_progress = static_cast<float>(i) / static_cast<float>(kOutputSize);
        if (new_progress - progress > 0.01f)
        {
            std::cout << '\r' << std::fixed << std::setprecision(2) << "Progress: " << 100.f * new_progress << "%"
                      << std::flush;
            progress = new_progress;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto render_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << std::endl;
    std::cout << "Simulation done" << std::endl;
    std::cout << "Render time: " << render_time << " ms" << std::endl;

    SF_INFO out_sf_info{0};
    out_sf_info.channels = 1;
    out_sf_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    out_sf_info.samplerate = static_cast<int>(kSampleRate);
    out_sf_info.frames = static_cast<sf_count_t>(kOutputSize);

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