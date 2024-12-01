#include "mesh_2d.h"

#include "rimguide.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>

#define IDX(x, y) ((x) + (y) * lx_)

Mesh2D::Mesh2D()
    : input_x(0)
    , input_y(0)
    , output_x(0)
    , output_y(0)
    , threadpool_(4)
    , sample_rate_(11025)
{
}

void Mesh2D::clear()
{
    for (auto& j : junctions_.container())
    {
        j.clear();
    }
}

Mat2D<uint8_t> Mesh2D::get_mask_for_radius(float radius) const
{
    Mat2D<uint8_t> mask;
    mask.allocate(lx_, ly_);

    for (size_t y = 0; y < ly_; ++y)
    {
        for (size_t x = 0; x < lx_; ++x)
        {
            const auto pos = junctions_(x, y).get_pos();
            const float dist = get_distance({0, 0}, pos);
            if (dist <= radius)
            {
                mask(x, y) = 1;
            }
            else
            {
                mask(x, y) = 0;
            }
        }
    }
    return mask;
}

void Mesh2D::init_boundary(const RimguideInfo& info)
{
    sample_rate_ = info.sample_rate;
    for (auto& j : junctions_.container())
    {
        if (j.is_boundary())
        {
            j.init_boundary(info);
            rimguides_.push_back(j.get_rimguide());
        }
    }
}

size_t Mesh2D::get_samplerate() const
{
    return sample_rate_;
}

float Mesh2D::get_energy() const
{
    float e = 0;
    for (const auto& j : junctions_.container())
    {
        e += j.get_energy();
    }

    return e;
}

void Mesh2D::set_input(float x, float y)
{
    x = std::clamp(x, 0.f, 1.f);
    y = std::clamp(y, 0.f, 1.f);

    input_x = static_cast<size_t>(x * lx_);
    input_y = static_cast<size_t>(y * ly_);
}

void Mesh2D::set_output(float x, float y)
{
    x = std::clamp(x, 0.f, 1.f);
    y = std::clamp(y, 0.f, 1.f);

    output_x = static_cast<size_t>(x * lx_);
    output_y = static_cast<size_t>(y * ly_);
}

void Mesh2D::set_absorption_coeff(float coeff)
{
    for (auto& j : junctions_.container())
    {
        j.set_absorption_coeff(coeff);
    }
}

float Mesh2D::tick(float input)
{
    // Found experimentally, this is the cutoff point where the multi-threaded version becomes faster
    constexpr size_t kGridSizeCutOff = 2000;

    if (junctions_.size() < kGridSizeCutOff)
    {
        return tick_st(input);
    }

    return tick_mt(input);
}

float Mesh2D::tick_st(float input)
{
    junctions_(input_x, input_y).add_input(input);

    for (auto& j : junctions_.container())
    {
        if (j.get_type() != 0)
        {
            j.process_scatter();
        }
    }

#ifdef SLOW_JUNCTION
    for (auto& j : junctions_.container())
    {
        j.process_delay();
    }
#endif

    return junctions_(output_x, output_y).get_output();
}

void Mesh2D::process_scatter_mt(size_t start, size_t end)
{
    // {
    //     std::lock_guard<std::mutex> lock(mtx);
    //     std::cout << "Scatter start: " << start << " end: " << end << std::endl;
    // }
    for (size_t i = start; i < end; ++i)
    {
        junctions_[i].process_scatter();
    }
}

void Mesh2D::process_delay_mt(size_t start, size_t end)
{
    // {
    //     std::lock_guard<std::mutex> lock(mtx);

    //     std::cout << "Delay start: " << start << " end: " << end << std::endl;
    // }
    for (size_t i = start; i < end; ++i)
    {
        junctions_[i].process_delay();
    }
}

float Mesh2D::tick_mt(float input)
{
    junctions_(input_x, input_y).add_input(input);

    const uint32_t n_threads = threadpool_.get_num_threads();
    std::vector<std::function<void()>> scatter_tasks;
    for (uint32_t i = 0; i < n_threads; ++i)
    {
        scatter_tasks.emplace_back([this, i, n_threads]() {
            process_scatter_mt(i * junctions_.size() / n_threads, (i + 1) * junctions_.size() / n_threads);
        });
    }
    threadpool_.enqueue_batch_and_wait(scatter_tasks);
    // std::cout << "Scatter pass done" << std::endl;

#ifdef SLOW_JUNCTION
    std::vector<std::function<void()>> delay_tasks;
    for (uint32_t i = 0; i < n_threads; ++i)
    {
        delay_tasks.emplace_back([this, i, n_threads]() {
            process_delay_mt(i * junctions_.size() / n_threads, (i + 1) * junctions_.size() / n_threads);
        });
    }
    threadpool_.enqueue_batch_and_wait(delay_tasks);
// std::cout << "Delay pass done" << std::endl;
#endif

    return junctions_(output_x, output_y).get_output();
}

Vec2Df Mesh2D::get_input_pos() const
{
    return junctions_(input_x, input_y).get_pos();
}

Vec2Df Mesh2D::get_output_pos() const
{
    return junctions_(output_x, output_y).get_pos();
}

void Mesh2D::get_junction_pressure(float* pressure) const
{
    for (const auto& j : junctions_.container())
    {
        *pressure = j.get_output();
        ++pressure;
    }
}

void Mesh2D::get_junction_types(uint8_t* types) const
{
    for (const auto& j : junctions_.container())
    {
        *types = j.get_type();
        ++types;
    }
}

size_t Mesh2D::get_junction_count() const
{
    size_t count = 0;
    for (const auto& j : junctions_.container())
    {
        if (j.get_type() != 0)
        {
            count++;
        }
    }

    return count;
}

size_t Mesh2D::get_rimguide_count() const
{
    size_t count = 0;
    for (const auto& j : junctions_.container())
    {
        if (j.has_rimguide())
        {
            count++;
        }
    }

    return count;
}

Rimguide* Mesh2D::get_rimguide(size_t idx)
{
    if (idx < rimguides_.size())
    {
        return rimguides_[idx];
    }
    return nullptr;
}
