#include "junction.h"

#include "rimguide.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

void Junction::init(JUNCTION_TYPE jtype, float x, float y)
{
    pos_.x = x;
    pos_.y = y;
    junction_type_ = jtype;

    switch (junction_type_)
    {
    case JUNCTION_TYPE::FOUR_PORT:
        neighbors_.resize(4, nullptr);
        in_.resize(4, 0.f);
        out_.resize(4, 0.f);
        break;
    case JUNCTION_TYPE::SIX_PORT:
        neighbors_.resize(6, nullptr);
        in_.resize(6, 0.f);
        out_.resize(6, 0.f);
        break;
    default:
        std::cerr << "Invalid junction type" << std::endl;
        break;
    }
}

Junction::Junction(Junction&& trijunction) noexcept
    : type_(trijunction.type_)
    , pos_(trijunction.pos_)
    , pressure_(trijunction.pressure_)
    , neighbors_(std::move(trijunction.neighbors_))
{

    if (trijunction.rimguide_ != nullptr)
    {
        rimguide_ = std::move(trijunction.rimguide_);
        trijunction.rimguide_ = nullptr;
    }
    num_connection_ = trijunction.num_connection_;
    in_ = trijunction.in_;
    out_ = trijunction.out_;
}

Junction& Junction::operator=(Junction&& trijunction) noexcept
{
    if (this != &trijunction)
    {
        type_ = trijunction.type_;
        pos_ = trijunction.pos_;
        pressure_ = trijunction.pressure_;
        type_ = trijunction.type_;
        neighbors_ = std::move(trijunction.neighbors_);
        if (trijunction.rimguide_ != nullptr)
        {
            rimguide_ = std::move(trijunction.rimguide_);
            trijunction.rimguide_ = nullptr;
        }
        num_connection_ = trijunction.num_connection_;
        in_ = trijunction.in_;
        out_ = trijunction.out_;
    }
    return *this;
}

Junction::~Junction()
{
    in_.clear();
    out_.clear();
    neighbors_.clear();
    rimguide_.reset();
}

void Junction::reset()
{
    for (auto& j : neighbors_)
    {
        j = nullptr;
    }
    clear();
}

void Junction::clear()
{
    for (size_t i = 0; i < in_.size(); ++i)
    {
        in_[i] = 0.f;
        out_[i] = 0.f;
    }
    input_ = 0.f;
    pressure_ = 0.f;

    if (rimguide_ != nullptr)
    {
        rimguide_->clear();
    }
}

void Junction::set_absorption_coeff(float abs_coeff)
{
    abs_coeff_ = abs_coeff;
}

void Junction::add_neighbor(Junction* neighbor, NEIGHBORS dir)
{
    assert(static_cast<size_t>(dir) < neighbors_.size());
    neighbors_[dir] = neighbor;
}

void Junction::init_junction_type()
{
    type_ = 0;

    for (size_t i = 0; i < neighbors_.size(); ++i)
    {
        if (neighbors_[i] != nullptr)
        {
            type_ |= (1 << i);
            num_connection_++;
        }
    }
}

void Junction::init_boundary(const RimguideInfo& info)
{
    assert(rimguide_ == nullptr);
    rimguide_ = std::make_unique<Rimguide>();
    rimguide_->init(info, this);
}

void Junction::init_inner_boundary()
{
    assert(rimguide_ == nullptr);
    rimguide_ = std::make_unique<Rimguide>();
    rimguide_->init_center();
}

void Junction::process_scatter()
{
#ifndef SLOW_JUNCTION

    if (use_alternate_)
    {
        if (junction_type_ == JUNCTION_TYPE::SIX_PORT)
        {
            process_scatter_6port_2();
        }
        else
        {
            process_scatter_4port_2();
        }
    }
    else
    {
        if (junction_type_ == JUNCTION_TYPE::SIX_PORT)
        {
            process_scatter_6port_1();
        }
        else
        {
            process_scatter_4port_1();
        }
    }
    if (rimguide_ != nullptr)
    {
        rimguide_->process_delay();
    }
    use_alternate_ = !use_alternate_;
#else

    float pj = 0.f;
    for (size_t i = 0; i < in_.size(); ++i)
    {
        pj += in_[i];
    }

    if (rimguide_ != nullptr)
    {
        pj += rimguide_->last_out() * (neighbors_.size() - num_connection_);
    }

    float scaler = 1.f;
    switch (junction_type_)
    {
    case JUNCTION_TYPE::FOUR_PORT:
        scaler = 1.f / 2.f;
        break;
    case JUNCTION_TYPE::SIX_PORT:
        scaler = 1.f / 3.f;
        break;
    default:
        std::cerr << "Invalid junction type" << std::endl;
        break;
    }

    pressure_ = pj * scaler + input_;

    float pj_out = 0.f;

    for (size_t i = 0; i < out_.size(); ++i)
    {
        if (type_[i] != 0)
        {
            out_[i] = pressure_ - in_[i];
            pj_out += out_[i];
        }
    }

    if (rimguide_ != nullptr)
    {
        const float rimguide_out = pressure_ - rimguide_->last_out();
        rimguide_->process_scatter(rimguide_out);
        pj_out += rimguide_out * (neighbors_.size() - num_connection_);
    }

    if (input_ == 0.f)
    {
        // Don't bother checking for energy conservation if there was an external input
        if (std::abs(pj_out - pj) > 1e-5)
        {
            std::cerr << "Energy not conserved" << std::endl;
        }
    }
    input_ = 0.f;
#endif
}

void Junction::process_scatter_4port_1()
{
    float pj = 0.f;
    float input_scaled = input_ * 0.5f;
    if (neighbors_[NEIGHBORS::NORTH] != nullptr)
    {
        pj += in_[NEIGHBORS::NORTH] + input_scaled;
    }
    assert(NEIGHBORS::SOUTH == 1);
    if (neighbors_[NEIGHBORS::SOUTH] != nullptr)
    {
        pj += in_[NEIGHBORS::SOUTH] + input_scaled;
    }
    assert(NEIGHBORS::EAST == 2);
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        pj += in_[NEIGHBORS::EAST] + input_scaled;
    }
    assert(NEIGHBORS::WEST == 3);
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        pj += in_[NEIGHBORS::WEST] + input_scaled;
    }

    if (rimguide_ != nullptr)
    {
        pj += rimguide_->last_out() * (neighbors_.size() - num_connection_);
    }

    float scaler = 1.f;
    switch (junction_type_)
    {
    case JUNCTION_TYPE::FOUR_PORT:
        scaler = 1.f / 2.f;
        break;
    case JUNCTION_TYPE::SIX_PORT:
        scaler = 1.f / 3.f;
        break;
    default:
        std::cerr << "Invalid junction type" << std::endl;
        break;
    }

    pressure_ = pj * scaler;

    float pj_out = 0.f;

    if (neighbors_[NEIGHBORS::NORTH] != nullptr)
    {
        out_[NEIGHBORS::NORTH] = (pressure_ - in_[NEIGHBORS::NORTH] - input_scaled);
        pj_out += out_[NEIGHBORS::NORTH];
    }
    assert(NEIGHBORS::SOUTH == 1);
    if (neighbors_[NEIGHBORS::SOUTH] != nullptr)
    {
        out_[NEIGHBORS::SOUTH] = (pressure_ - in_[NEIGHBORS::SOUTH] - input_scaled);
        pj_out += out_[NEIGHBORS::SOUTH];
    }
    assert(NEIGHBORS::EAST == 2);
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        out_[NEIGHBORS::EAST] = (pressure_ - in_[NEIGHBORS::EAST] - input_scaled);
        pj_out += out_[NEIGHBORS::EAST];
    }
    assert(NEIGHBORS::WEST == 3);
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        out_[NEIGHBORS::WEST] = (pressure_ - in_[NEIGHBORS::WEST] - input_scaled);
        pj_out += out_[NEIGHBORS::WEST];
    }

    if (rimguide_ != nullptr)
    {
        const float rimguide_out = pressure_ - rimguide_->last_out();
        rimguide_->process_scatter(rimguide_out);
        pj_out += rimguide_out * (neighbors_.size() - num_connection_);
    }

    if (input_ == 0.f)
    {
        // Don't bother checking for energy conservation if there was an external input
        if (std::abs(pj_out - pj) > 1e-5)
        {
            std::cerr << "Energy not conserved" << std::endl;
        }
    }
    input_ = 0.f;
}

void Junction::process_scatter_4port_2()
{
    float pj = 0.f;
    float input_scaled = input_ * 0.5f;
    if (neighbors_[NEIGHBORS::NORTH] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::NORTH]->out_[NEIGHBORS::SOUTH] + input_scaled;
    }
    assert(NEIGHBORS::SOUTH == 1);
    if (neighbors_[NEIGHBORS::SOUTH] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::SOUTH]->out_[NEIGHBORS::NORTH] + input_scaled;
    }
    assert(NEIGHBORS::EAST == 2);
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::EAST]->out_[NEIGHBORS::WEST] + input_scaled;
    }
    assert(NEIGHBORS::WEST == 3);
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::WEST]->out_[NEIGHBORS::EAST] + input_scaled;
    }

    if (rimguide_ != nullptr)
    {
        pj += rimguide_->last_out() * (neighbors_.size() - num_connection_);
    }

    float scaler = 1.f;
    switch (junction_type_)
    {
    case JUNCTION_TYPE::FOUR_PORT:
        scaler = 1.f / 2.f;
        break;
    case JUNCTION_TYPE::SIX_PORT:
        scaler = 1.f / 3.f;
        break;
    default:
        std::cerr << "Invalid junction type" << std::endl;
        break;
    }

    pressure_ = pj * scaler;

    float pj_out = 0.f;

    if (neighbors_[NEIGHBORS::NORTH] != nullptr)
    {
        neighbors_[NEIGHBORS::NORTH]->in_[NEIGHBORS::SOUTH] =
            (pressure_ - neighbors_[NEIGHBORS::NORTH]->out_[NEIGHBORS::SOUTH] - input_scaled);
        pj_out += neighbors_[NEIGHBORS::NORTH]->in_[NEIGHBORS::SOUTH];
    }
    assert(NEIGHBORS::SOUTH == 1);
    if (neighbors_[NEIGHBORS::SOUTH] != nullptr)
    {
        neighbors_[NEIGHBORS::SOUTH]->in_[NEIGHBORS::NORTH] =
            (pressure_ - neighbors_[NEIGHBORS::SOUTH]->out_[NEIGHBORS::NORTH] - input_scaled);

        pj_out += neighbors_[NEIGHBORS::SOUTH]->in_[NEIGHBORS::NORTH];
    }
    assert(NEIGHBORS::EAST == 2);
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        neighbors_[NEIGHBORS::EAST]->in_[NEIGHBORS::WEST] =
            (pressure_ - neighbors_[NEIGHBORS::EAST]->out_[NEIGHBORS::WEST] - input_scaled);
        pj_out += neighbors_[NEIGHBORS::EAST]->in_[NEIGHBORS::WEST];
    }
    assert(NEIGHBORS::WEST == 3);
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        neighbors_[NEIGHBORS::WEST]->in_[NEIGHBORS::EAST] =
            (pressure_ - neighbors_[NEIGHBORS::WEST]->out_[NEIGHBORS::EAST] - input_scaled);
        pj_out += neighbors_[NEIGHBORS::WEST]->in_[NEIGHBORS::EAST];
    }

    if (rimguide_ != nullptr)
    {
        const float rimguide_out = pressure_ - rimguide_->last_out();
        rimguide_->process_scatter(rimguide_out);
        pj_out += rimguide_out * (neighbors_.size() - num_connection_);
    }

    if (input_ == 0.f)
    {
        // Don't bother checking for energy conservation if there was an external input
        if (std::abs(pj_out - pj) > 1e-5)
        {
            std::cerr << "Energy not conserved" << std::endl;
        }
    }
    input_ = 0.f;
}

void Junction::process_scatter_6port_1()
{
    float pj = 0.f;
    float input_scaled = input_ / 3.f;
    if (neighbors_[NEIGHBORS::NORTH_EAST] != nullptr)
    {
        pj += in_[NEIGHBORS::NORTH_EAST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::NORTH_WEST] != nullptr)
    {
        pj += in_[NEIGHBORS::NORTH_WEST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::SOUTH_EAST] != nullptr)
    {
        pj += in_[NEIGHBORS::SOUTH_EAST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::SOUTH_WEST] != nullptr)
    {
        pj += in_[NEIGHBORS::SOUTH_WEST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        pj += in_[NEIGHBORS::EAST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        pj += in_[NEIGHBORS::WEST] + input_scaled;
    }

    if (rimguide_ != nullptr)
    {
        pj += rimguide_->last_out() * (neighbors_.size() - num_connection_);
    }

    float scaler = 1.f;
    switch (junction_type_)
    {
    case JUNCTION_TYPE::FOUR_PORT:
        scaler = 1.f / 2.f;
        break;
    case JUNCTION_TYPE::SIX_PORT:
        scaler = 1.f / 3.f;
        break;
    default:
        std::cerr << "Invalid junction type" << std::endl;
        break;
    }

    pressure_ = pj * scaler;

    float pj_out = 0.f;

    if (neighbors_[NEIGHBORS::NORTH_EAST] != nullptr)
    {
        out_[NEIGHBORS::NORTH_EAST] = pressure_ - in_[NEIGHBORS::NORTH_EAST] - input_scaled;
        pj_out += out_[NEIGHBORS::NORTH_EAST];
    }
    if (neighbors_[NEIGHBORS::NORTH_WEST] != nullptr)
    {
        out_[NEIGHBORS::NORTH_WEST] = pressure_ - in_[NEIGHBORS::NORTH_WEST] - input_scaled;
        pj_out += out_[NEIGHBORS::NORTH_WEST];
    }
    if (neighbors_[NEIGHBORS::SOUTH_EAST] != nullptr)
    {
        out_[NEIGHBORS::SOUTH_EAST] = pressure_ - in_[NEIGHBORS::SOUTH_EAST] - input_scaled;
        pj_out += out_[NEIGHBORS::SOUTH_EAST];
    }
    if (neighbors_[NEIGHBORS::SOUTH_WEST] != nullptr)
    {
        out_[NEIGHBORS::SOUTH_WEST] = pressure_ - in_[NEIGHBORS::SOUTH_WEST] - input_scaled;
        pj_out += out_[NEIGHBORS::SOUTH_WEST];
    }
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        out_[NEIGHBORS::EAST] = pressure_ - in_[NEIGHBORS::EAST] - input_scaled;
        pj_out += out_[NEIGHBORS::EAST];
    }
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        out_[NEIGHBORS::WEST] = pressure_ - in_[NEIGHBORS::WEST] - input_scaled;
        pj_out += out_[NEIGHBORS::WEST];
    }

    if (rimguide_ != nullptr)
    {
        const float rimguide_out = pressure_ - rimguide_->last_out();
        rimguide_->process_scatter(rimguide_out);
        pj_out += rimguide_out * (neighbors_.size() - num_connection_);
    }

    if (input_ == 0.f)
    {
        // Don't bother checking for energy conservation if there was an external input
        if (std::abs(pj_out - pj) > 1e-5)
        {
            std::cerr << "Energy not conserved" << std::endl;
        }
    }
    input_ = 0.f;
}

void Junction::process_scatter_6port_2()
{
    float pj = 0.f;
    float input_scaled = input_ / 3.f;
    if (neighbors_[NEIGHBORS::NORTH_EAST] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::NORTH_EAST]->out_[NEIGHBORS::SOUTH_WEST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::NORTH_WEST] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::NORTH_WEST]->out_[NEIGHBORS::SOUTH_EAST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::SOUTH_EAST] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::SOUTH_EAST]->out_[NEIGHBORS::NORTH_WEST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::SOUTH_WEST] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::SOUTH_WEST]->out_[NEIGHBORS::NORTH_EAST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::EAST]->out_[NEIGHBORS::WEST] + input_scaled;
    }
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        pj += neighbors_[NEIGHBORS::WEST]->out_[NEIGHBORS::EAST] + input_scaled;
    }

    if (rimguide_ != nullptr)
    {
        pj += rimguide_->last_out() * (neighbors_.size() - num_connection_);
    }

    float scaler = 1.f;
    switch (junction_type_)
    {
    case JUNCTION_TYPE::FOUR_PORT:
        scaler = 1.f / 2.f;
        break;
    case JUNCTION_TYPE::SIX_PORT:
        scaler = 1.f / 3.f;
        break;
    default:
        std::cerr << "Invalid junction type" << std::endl;
        break;
    }

    pressure_ = pj * scaler;

    float pj_out = 0.f;

    if (neighbors_[NEIGHBORS::NORTH_EAST] != nullptr)
    {
        neighbors_[NEIGHBORS::NORTH_EAST]->in_[NEIGHBORS::SOUTH_WEST] =
            pressure_ - neighbors_[NEIGHBORS::NORTH_EAST]->out_[NEIGHBORS::SOUTH_WEST] - input_scaled;
        pj_out += neighbors_[NEIGHBORS::NORTH_EAST]->in_[NEIGHBORS::SOUTH_WEST];
    }
    if (neighbors_[NEIGHBORS::NORTH_WEST] != nullptr)
    {
        neighbors_[NEIGHBORS::NORTH_WEST]->in_[NEIGHBORS::SOUTH_EAST] =
            pressure_ - neighbors_[NEIGHBORS::NORTH_WEST]->out_[NEIGHBORS::SOUTH_EAST] - input_scaled;
        pj_out += neighbors_[NEIGHBORS::NORTH_WEST]->in_[NEIGHBORS::SOUTH_EAST];
    }
    if (neighbors_[NEIGHBORS::SOUTH_EAST] != nullptr)
    {
        neighbors_[NEIGHBORS::SOUTH_EAST]->in_[NEIGHBORS::NORTH_WEST] =
            pressure_ - neighbors_[NEIGHBORS::SOUTH_EAST]->out_[NEIGHBORS::NORTH_WEST] - input_scaled;

        pj_out += neighbors_[NEIGHBORS::SOUTH_EAST]->in_[NEIGHBORS::NORTH_WEST];
    }
    if (neighbors_[NEIGHBORS::SOUTH_WEST] != nullptr)
    {
        neighbors_[NEIGHBORS::SOUTH_WEST]->in_[NEIGHBORS::NORTH_EAST] =
            pressure_ - neighbors_[NEIGHBORS::SOUTH_WEST]->out_[NEIGHBORS::NORTH_EAST] - input_scaled;

        pj_out += neighbors_[NEIGHBORS::SOUTH_WEST]->in_[NEIGHBORS::NORTH_EAST];
    }
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        neighbors_[NEIGHBORS::EAST]->in_[NEIGHBORS::WEST] =
            pressure_ - neighbors_[NEIGHBORS::EAST]->out_[NEIGHBORS::WEST] - input_scaled;
        pj_out += neighbors_[NEIGHBORS::EAST]->in_[NEIGHBORS::WEST];
    }
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        neighbors_[NEIGHBORS::WEST]->in_[NEIGHBORS::EAST] =
            pressure_ - neighbors_[NEIGHBORS::WEST]->out_[NEIGHBORS::EAST] - input_scaled;
        pj_out += neighbors_[NEIGHBORS::WEST]->in_[NEIGHBORS::EAST];
    }

    if (rimguide_ != nullptr)
    {
        const float rimguide_out = pressure_ - rimguide_->last_out();
        rimguide_->process_scatter(rimguide_out);
        pj_out += rimguide_out * (neighbors_.size() - num_connection_);
    }

    if (input_ == 0.f)
    {
        // Don't bother checking for energy conservation if there was an external input
        if (std::abs(pj_out - pj) > 1e-5)
        {
            std::cerr << "Energy not conserved" << std::endl;
        }
    }
    input_ = 0.f;
}

void Junction::process_delay()
{
    switch (junction_type_)
    {
    case JUNCTION_TYPE::FOUR_PORT:
        process_delay_four_port();
        break;
    case JUNCTION_TYPE::SIX_PORT:
        process_delay_six_port();
        break;
    default:
        std::cerr << "Invalid junction type" << std::endl;
        break;
    }

    if (rimguide_ != nullptr)
    {
        rimguide_->process_delay();
    }
}

void Junction::process_delay_six_port()
{
    assert(neighbors_.size() == 6);
    if (neighbors_[NEIGHBORS::NORTH_EAST] != nullptr)
    {
        in_[NEIGHBORS::NORTH_EAST] = neighbors_[NEIGHBORS::NORTH_EAST]->out_[NEIGHBORS::SOUTH_WEST];
    }
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        in_[NEIGHBORS::EAST] = neighbors_[NEIGHBORS::EAST]->out_[NEIGHBORS::WEST];
    }
    if (neighbors_[NEIGHBORS::SOUTH_EAST] != nullptr)
    {
        in_[NEIGHBORS::SOUTH_EAST] = neighbors_[NEIGHBORS::SOUTH_EAST]->out_[NEIGHBORS::NORTH_WEST];
    }
    if (neighbors_[NEIGHBORS::SOUTH_WEST] != nullptr)
    {
        in_[NEIGHBORS::SOUTH_WEST] = neighbors_[NEIGHBORS::SOUTH_WEST]->out_[NEIGHBORS::NORTH_EAST];
    }
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        in_[NEIGHBORS::WEST] = neighbors_[NEIGHBORS::WEST]->out_[NEIGHBORS::EAST];
    }
    if (neighbors_[NEIGHBORS::NORTH_WEST] != nullptr)
    {
        in_[NEIGHBORS::NORTH_WEST] = neighbors_[NEIGHBORS::NORTH_WEST]->out_[NEIGHBORS::SOUTH_EAST];
    }
}

void Junction::process_delay_four_port()
{
    assert(neighbors_.size() == 4);
    assert(NEIGHBORS::NORTH == 0);
    if (neighbors_[NEIGHBORS::NORTH] != nullptr)
    {
        in_[NEIGHBORS::NORTH] = neighbors_[NEIGHBORS::NORTH]->out_[NEIGHBORS::SOUTH];
    }
    assert(NEIGHBORS::SOUTH == 1);
    if (neighbors_[NEIGHBORS::SOUTH] != nullptr)
    {
        in_[NEIGHBORS::SOUTH] = neighbors_[NEIGHBORS::SOUTH]->out_[NEIGHBORS::NORTH];
    }
    assert(NEIGHBORS::EAST == 2);
    if (neighbors_[NEIGHBORS::EAST] != nullptr)
    {
        in_[NEIGHBORS::EAST] = neighbors_[NEIGHBORS::EAST]->out_[NEIGHBORS::WEST];
    }
    assert(NEIGHBORS::WEST == 3);
    if (neighbors_[NEIGHBORS::WEST] != nullptr)
    {
        in_[NEIGHBORS::WEST] = neighbors_[NEIGHBORS::WEST]->out_[NEIGHBORS::EAST];
    }
}

void Junction::add_input(float input)
{
    input_ += input;
}

float Junction::get_output() const
{
    return pressure_;
}

bool Junction::has_rimguide() const
{
    return rimguide_ != nullptr;
}

const Rimguide* Junction::get_rimguide() const
{
    return rimguide_.get();
}

Rimguide* Junction::get_rimguide()
{
    return rimguide_.get();
}

uint32_t Junction::get_type() const
{
    return type_.to_ulong();
}

Vec2Df Junction::get_pos() const
{
    return pos_;
}

float Junction::get_energy() const
{
    float e = 0.f;
    for (size_t i = 0; i < in_.size(); ++i)
    {
        e += in_[i] * in_[i];
    }
    return e;
}

Junction* Junction::get_neighbor(NEIGHBORS dir) const
{
    assert(static_cast<size_t>(dir) < neighbors_.size());
    return neighbors_[dir];
}

void Junction::remove_neighbor(NEIGHBORS dir)
{
    assert(static_cast<size_t>(dir) < neighbors_.size());
    neighbors_[dir] = nullptr;
    type_[static_cast<size_t>(dir)] = false;
    num_connection_--;
}

bool Junction::is_boundary() const
{
    if (junction_type_ == JUNCTION_TYPE::FOUR_PORT)
    {
        return num_connection_ < 4 && num_connection_ > 0;
    }
    if (junction_type_ == JUNCTION_TYPE::SIX_PORT)
    {
        return num_connection_ < 6 && num_connection_ > 0;
    }

    return false;
}

void Junction::print_info() const
{
    std::cout << "Pos: " << pos_.x << ", " << pos_.y << std::endl;
    std::cout << "Type: " << type_ << std::endl;
    std::cout << "Neighbors: " << std::endl;
    for (size_t i = 0; i < neighbors_.size(); ++i)
    {
        if (neighbors_[i] != nullptr)
        {
            std::string dir;
            switch (i)
            {
            case NEIGHBORS::EAST:
                dir = "EAST";
                break;
            case NEIGHBORS::WEST:
                dir = "WEST";
                break;
            case NEIGHBORS::NORTH_WEST:
                dir = "NORTH_WEST";
                break;
            case NEIGHBORS::NORTH_EAST:
                dir = "NORTH_EAST";
                break;
            case NEIGHBORS::SOUTH_WEST:
                dir = "SOUTH_WEST";
                break;
            case NEIGHBORS::SOUTH_EAST:
                dir = "SOUTH_EAST";
                break;
            default:
                dir = "UNKNOWN";
                break;
            }
            std::cout << "  " << dir << ": " << neighbors_[i]->get_pos().x << ", " << neighbors_[i]->get_pos().y
                      << std::endl;
        }
    }
}