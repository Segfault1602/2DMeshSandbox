#include "listener.h"

#include "DelayA.h"
#include "junction.h"
#include "mat2d.h"
#include "mesh_2d.h"
#include "rimguide.h"
#include "vec2d.h"

#include <cassert>
#include <iostream>

namespace
{
constexpr float kSpeedOfSoundInAir = 343.0f;
}

Listener::Listener()
    : mesh_(nullptr)
{
}

void Listener::init(const Mesh2D& mesh, const ListenerInfo& info)
{
    mesh_ = &mesh;
    pos_ = info.position;
    delays_.reserve(mesh.get_junction_count());
    loss_factors_.reserve(mesh.get_junction_count());
    type_ = info.type;

    const float sample_distance = kSpeedOfSoundInAir / info.samplerate;

    for (const auto& j : mesh_->junctions_.container())
    {
        if (j.get_type() == 0)
        {
            continue;
        }

        if (type_ == ListenerType::BOUNDARY && !j.is_boundary())
        {
            continue;
        }

        if (type_ == ListenerType::POINT)
        {
            if (j.get_pos() == Vec2Df{pos_.x, pos_.y})
            {
                point_source_ = &j;
                break;
            }

            continue;
        }

        if (type_ == ListenerType::ZONE)
        {
            const float distance = get_distance(j.get_pos(), {pos_.x, pos_.y});
            if (distance > info.radius)
            {
                continue;
            }
        }
        Vec3Df junction_pos = {j.get_pos().x, j.get_pos().y, 0.0f};
        float distance = get_distance(junction_pos, pos_);

        float delay = distance / sample_distance;
        delays_.emplace_back(delay, static_cast<unsigned long>(delay + 8));
        loss_factors_.push_back(sample_distance / (distance));
    }

    if (delays_.empty())
    {
        std::cerr << "No junctions found for listener" << std::endl;
    }
}

void Listener::set_gain(float gain)
{
    gain_ = gain;
}

float Listener::tick()
{
    if (type_ == ListenerType::POINT)
    {
        assert(point_source_ != nullptr);
        return point_source_->get_output();
    }

    size_t i = 0;
    float out = 0.f;
    for (const auto& j : mesh_->junctions_.container())
    {
        float junction_output = 0.f;
        if (j.get_type() == 0)
        {
            continue;
        }
        if (type_ == ListenerType::BOUNDARY)
        {
            if (!j.is_boundary())
            {
                continue;
            }
            assert(j.has_rimguide());
            junction_output = j.get_output(); //.get_rimguide()->last_out();
        }
        else if (type_ == ListenerType::POINT && j.get_pos() != Vec2Df{pos_.x, pos_.y})
        {
            continue;
        }
        else
        {
            junction_output = j.get_output();
        }

        out += delays_[i].tick(junction_output) * loss_factors_[i];
        i++;
    }

    return out * gain_;
}