#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "gaussian.h"
#include "mat2d.h"
#include "nanobench.h"
#include "rectilinear_mesh.h"
#include "rimguide.h"
#include "trimesh.h"
#include "wave_math.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <format>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

using namespace ankerl;
using namespace std::chrono_literals;

namespace
{
constexpr float kSampleRate = 11025;

constexpr float kDensity = 0.262;
constexpr float kRadius = 0.32; // radius of the membrane

constexpr float kTension = 3325.f;
constexpr float kThickness = 0.0003f; // thickness of the membrane
constexpr float kDecay = 25.f;

constexpr size_t kIterationCount = kSampleRate;

} // namespace

TEST_CASE("TriMesh")
{
    float c = get_wave_speed(kTension, kDensity);
    float sample_distance = get_sample_distance(c, kSampleRate);
    float f0 = get_fundamental_frequency(kRadius, c, kSampleRate);
    float f0_hz = f0 * kSampleRate / (2 * M_PI);
    float friction_coeff = get_friction_coeff(kRadius, c, kDecay, f0);
    float friction_delay = get_friction_delay(friction_coeff, f0);
    float max_radius = get_max_radius(kRadius, friction_delay, sample_distance);
    auto grid_size = get_grid_size(max_radius, sample_distance, 2.f / std::numbers::sqrt3_v<float>);

    const size_t kGridX = grid_size[0];
    const size_t kGridY = grid_size[1];

    RimguideInfo info{};
    info.radius = kRadius;
    info.friction_coeff = -friction_coeff;
    info.friction_delay = friction_delay;
    info.wave_speed = c;
    info.sample_rate = kSampleRate;
    info.is_solid_boundary = true;

    TriMesh mesh(kGridX, kGridY, sample_distance);
    auto mask = mesh.get_mask_for_radius(max_radius);

    mesh.init(mask);
    mesh.init_boundary(info);

    mesh.set_input(0.5, 0.5);
    mesh.set_output(0.5, 0.5);

    auto impulse = raised_cosine(100, kSampleRate);

    std::string title = std::format("Trimesh - {} hz", kSampleRate);
    nanobench::Bench bench;
    bench.title(title);
    bench.relative(true);
    bench.performanceCounters(true);
    // bench.minEpochIterations(5);
    bench.timeUnit(1ms, "ms");

    bench.run("Trimesh - Single Thread", [&] {
        for (auto i = 0; i < kIterationCount - 1; i++)
        {
            float input = 0.f;
            if (i < impulse.size())
            {
                input = -impulse[i];
            }
            float out = mesh.tick(input);
            ankerl::nanobench::doNotOptimizeAway(out);
        }
    });

    TriMesh trimesh_mt(kGridX, kGridY, sample_distance);
    mask = trimesh_mt.get_mask_for_radius(max_radius);
    trimesh_mt.init(mask);
    trimesh_mt.init_boundary(info);
    trimesh_mt.set_input(0.5, 0.5);
    trimesh_mt.set_output(0.5, 0.5);

    bench.run("Trimesh - Multi Thread", [&] {
        for (auto i = 0; i < kIterationCount - 1; i++)
        {
            float input = 0.f;
            if (i < impulse.size())
            {
                input = -impulse[i];
            }
            float out = trimesh_mt.tick_mt(input);
            ankerl::nanobench::doNotOptimizeAway(out);
        }
    });
}

TEST_CASE("Rectangular mesh")
{
    float c = get_wave_speed(kTension, kDensity);
    float sample_distance = get_sample_distance(c, kSampleRate);
    float f0 = get_fundamental_frequency(kRadius, c, kSampleRate);
    float f0_hz = f0 * kSampleRate / (2 * M_PI);
    float friction_coeff = get_friction_coeff(kRadius, c, kDecay, f0);
    float friction_delay = get_friction_delay(friction_coeff, f0);
    float max_radius = get_max_radius(kRadius, friction_delay, sample_distance);
    auto grid_size = get_grid_size(max_radius, sample_distance, 2.f / std::numbers::sqrt3_v<float>);

    const size_t kGridX = grid_size[0];
    const size_t kGridY = grid_size[1];

    RimguideInfo info{};
    info.radius = kRadius;
    info.friction_coeff = -friction_coeff;
    info.friction_delay = friction_delay;
    info.wave_speed = c;
    info.sample_rate = kSampleRate;
    info.is_solid_boundary = true;

    RectilinearMesh rect_mesh(kGridX, kGridY, sample_distance);
    auto mask = rect_mesh.get_mask_for_radius(max_radius);
    rect_mesh.init(mask);
    rect_mesh.init_boundary(info);
    rect_mesh.set_input(0.5, 0.5);
    rect_mesh.set_output(0.5, 0.5);

    auto impulse = raised_cosine(100, kSampleRate);

    nanobench::Bench bench;
    std::string title = std::format("Rectangular mesh - {} hz", kSampleRate);

    bench.title(title);
    bench.relative(true);
    // bench.performanceCounters(true);
    // bench.minEpochIterations(5);
    bench.timeUnit(1ms, "ms");

    bench.run("RectMesh - Single thread", [&] {
        for (auto i = 0; i < kIterationCount - 1; i++)
        {
            float input = 0.f;
            if (i < impulse.size())
            {
                input = -impulse[i];
            }
            float out = rect_mesh.tick(input);
            ankerl::nanobench::doNotOptimizeAway(out);
        }
    });
    RectilinearMesh rect_mesh_mt(kGridX, kGridY, sample_distance);
    mask = rect_mesh_mt.get_mask_for_radius(max_radius);
    rect_mesh_mt.init(mask);
    rect_mesh_mt.init_boundary(info);
    rect_mesh_mt.set_input(0.5, 0.5);
    rect_mesh_mt.set_output(0.5, 0.5);

    bench.run("RectMesh - Multi Thread", [&] {
        for (auto i = 0; i < kIterationCount - 1; i++)
        {
            float input = 0.f;
            if (i < impulse.size())
            {
                input = -impulse[i];
            }
            float out = rect_mesh_mt.tick_mt(input);
            ankerl::nanobench::doNotOptimizeAway(out);
        }
    });
}

TEST_CASE("TriMesh single thread- BigO")
{
    std::string title = std::format("Trimesh single thread- BigO", kSampleRate);
    nanobench::Bench bench;
    bench.title(title);
    bench.timeUnit(1ms, "ms");
    bench.relative(true);
    auto impulse = raised_cosine(100, kSampleRate);

    for (auto radius = 0.05; radius < 0.95; radius += 0.05)
    {
        float c = get_wave_speed(kTension, kDensity);
        float sample_distance = get_sample_distance(c, kSampleRate);
        float f0 = get_fundamental_frequency(radius, c, kSampleRate);
        float f0_hz = f0 * kSampleRate / (2 * M_PI);
        float friction_coeff = get_friction_coeff(radius, c, kDecay, f0);
        float friction_delay = get_friction_delay(friction_coeff, f0);
        float max_radius = get_max_radius(radius, friction_delay, sample_distance);
        auto grid_size = get_grid_size(max_radius, sample_distance, 2.f / std::numbers::sqrt3_v<float>);

        const size_t kGridX = grid_size[0];
        const size_t kGridY = grid_size[1];

        RimguideInfo info{};
        info.radius = radius;
        info.friction_coeff = -friction_coeff;
        info.friction_delay = friction_delay;
        info.wave_speed = c;
        info.sample_rate = kSampleRate;
        info.is_solid_boundary = true;

        TriMesh mesh(kGridX, kGridY, sample_distance);
        auto mask = mesh.get_mask_for_radius(max_radius);

        mesh.init(mask);
        mesh.init_boundary(info);

        mesh.set_input(0.5, 0.5);
        mesh.set_output(0.5, 0.5);

        auto total_grid_size = kGridX * kGridY;

        std::string title = std::format("Trimesh - {}", radius);
        bench.complexityN(total_grid_size).run(title, [&] {
            for (auto i = 0; i < kIterationCount - 1; i++)
            {
                float input = 0.f;
                if (i < impulse.size())
                {
                    input = -impulse[i];
                }
                float out = mesh.tick_st(input);
                ankerl::nanobench::doNotOptimizeAway(out);
            }
        });
    }

    // calculate BigO complexy best fit and print the results
    std::cout << bench.complexityBigO() << std::endl;
}

TEST_CASE("TriMesh multi thread- BigO")
{
    std::string title = std::format("Trimesh multi thread- BigO", kSampleRate);
    nanobench::Bench bench;
    bench.title(title);
    bench.timeUnit(1ms, "ms");
    bench.relative(true);
    auto impulse = raised_cosine(100, kSampleRate);

    for (auto radius = 0.05; radius < 0.95; radius += 0.05)
    {
        float c = get_wave_speed(kTension, kDensity);
        float sample_distance = get_sample_distance(c, kSampleRate);
        float f0 = get_fundamental_frequency(radius, c, kSampleRate);
        float f0_hz = f0 * kSampleRate / (2 * M_PI);
        float friction_coeff = get_friction_coeff(radius, c, kDecay, f0);
        float friction_delay = get_friction_delay(friction_coeff, f0);
        float max_radius = get_max_radius(radius, friction_delay, sample_distance);
        auto grid_size = get_grid_size(max_radius, sample_distance, 2.f / std::numbers::sqrt3_v<float>);

        const size_t kGridX = grid_size[0];
        const size_t kGridY = grid_size[1];

        RimguideInfo info{};
        info.radius = radius;
        info.friction_coeff = -friction_coeff;
        info.friction_delay = friction_delay;
        info.wave_speed = c;
        info.sample_rate = kSampleRate;
        info.is_solid_boundary = true;

        TriMesh mesh(kGridX, kGridY, sample_distance);
        auto mask = mesh.get_mask_for_radius(max_radius);

        mesh.init(mask);
        mesh.init_boundary(info);

        mesh.set_input(0.5, 0.5);
        mesh.set_output(0.5, 0.5);

        auto total_grid_size = kGridX * kGridY;

        std::string title = std::format("Trimesh - {}", radius);
        bench.complexityN(total_grid_size).run(title, [&] {
            for (auto i = 0; i < kIterationCount - 1; i++)
            {
                float input = 0.f;
                if (i < impulse.size())
                {
                    input = -impulse[i];
                }
                float out = mesh.tick_mt(input);
                ankerl::nanobench::doNotOptimizeAway(out);
            }
        });
    }

    // calculate BigO complexy best fit and print the results
    std::cout << bench.complexityBigO() << std::endl;
}
