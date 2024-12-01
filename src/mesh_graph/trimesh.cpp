#include "trimesh.h"

#include "junction.h"
#include "mat2d.h"
#include "vec2d.h"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <vector>

#define IDX(x, y) ((x) + (y) * lx_)

TriMesh::TriMesh(size_t lx, size_t ly, float sample_distance)
{
    lx_ = lx;
    ly_ = ly;

    if (lx_ == 0 || ly_ == 0)
    {
        std::cerr << "Invalid mesh size" << std::endl;
        lx_ = 1;
        ly_ = 1;
    }

    junctions_.allocate(lx_, ly_);

    constexpr float offset = 0.5f;

    float x_offset = -floor(lx_ / 2.f);
    const float y_offset = -floor((ly_ / 2.f));

    size_t middle_row = ly_ / 2;
    if (middle_row % 2 == 1)
    {
        x_offset += 0.5f;
    }

    for (size_t y = ly_; y > 0; --y)
    {
        const bool is_odd = (y - 1) % 2 != 0;
        for (size_t x = 0; x < lx_; ++x)
        {
            float x_pos = x - (is_odd * offset) + x_offset;
            float y_pos = (y - 1) + y_offset;

            // Scale the positions based on the sample distance
            x_pos *= sample_distance;
            y_pos *= sample_distance * std::numbers::sqrt3_v<float> / 2.f;

            junctions_(x, y - 1).init(JUNCTION_TYPE::SIX_PORT, x_pos, y_pos);
        }
    }

    // Shift the positions to center the mesh
}

void TriMesh::init(const Mat2D<uint8_t>& mask)
{
    const size_t size = lx_ * ly_;

    if (mask.size() != size)
    {
        std::cerr << "Mask size does not match mesh size" << std::endl;
        return;
    }

    for (size_t y = 0; y < ly_; ++y)
    {
        const bool is_odd = y % 2 != 0;
        for (size_t x = 0; x < lx_; ++x)
        {
            if (mask(x, y) == 0)
            {
                continue;
            }

            if (x > 0 && mask(x - 1, y) == 1)
            {
                junctions_(x, y).add_neighbor(&junctions_(x - 1, y), WEST);
            }
            if (x < lx_ - 1 && mask(x + 1, y) == 1)
            {
                junctions_(x, y).add_neighbor(&junctions_(x + 1, y), EAST);
            }

            if (is_odd)
            {
                if (y < ly_ - 1 && x > 0 && mask(x - 1, y + 1) == 1)
                {
                    junctions_(x, y).add_neighbor(&junctions_(x - 1, y + 1), NORTH_WEST);
                }
                if (y < ly_ - 1 && mask(x, y + 1) == 1)
                {
                    junctions_(x, y).add_neighbor(&junctions_(x, y + 1), NORTH_EAST);
                }
                if (y > 0 && x > 0 && mask(x - 1, y - 1) == 1)
                {
                    junctions_(x, y).add_neighbor(&junctions_(x - 1, y - 1), SOUTH_WEST);
                }
                if (y > 0 && mask(x, y - 1) == 1)
                {
                    junctions_(x, y).add_neighbor(&junctions_(x, y - 1), SOUTH_EAST);
                }
            }
            else
            {
                if (y < ly_ - 1 && mask(x, y + 1) == 1)
                {
                    junctions_(x, y).add_neighbor(&junctions_(x, y + 1), NORTH_WEST);
                }
                if (y < ly_ - 1 && x < lx_ - 1 && mask(x + 1, y + 1) == 1)
                {
                    junctions_(x, y).add_neighbor(&junctions_(x + 1, y + 1), NORTH_EAST);
                }
                if (y > 0 && mask(x, y - 1) == 1)
                {
                    junctions_(x, y).add_neighbor(&junctions_(x, y - 1), SOUTH_WEST);
                }
                if (y > 0 && x < lx_ - 1 && mask(x + 1, y - 1) == 1)
                {
                    junctions_(x, y).add_neighbor(&junctions_(x + 1, y - 1), SOUTH_EAST);
                }
            }
        }
    }

    for (auto& j : junctions_.container())
    {
        j.init_junction_type();
    }
}

void TriMesh::clamp_center_with_rimguide()
{
    Junction* center = nullptr;

    for (auto& j : junctions_.container())
    {
        // Not ideal, but should work for now as we always make sure there is a node at (0,0)
        // when creating the mesh
        if (j.get_pos() == Vec2Df{0, 0})
        {
            center = &j;
            break;
        }
    }

    assert(center != nullptr);
    if (!center)
    {
        std::cerr << "Center junction not found" << std::endl;
    }

    center->get_neighbor(NEIGHBORS::EAST)->remove_neighbor(NEIGHBORS::WEST);
    center->get_neighbor(NEIGHBORS::NORTH_EAST)->remove_neighbor(NEIGHBORS::SOUTH_WEST);
    center->get_neighbor(NEIGHBORS::SOUTH_EAST)->remove_neighbor(NEIGHBORS::NORTH_WEST);
    center->get_neighbor(NEIGHBORS::WEST)->remove_neighbor(NEIGHBORS::EAST);
    center->get_neighbor(NEIGHBORS::NORTH_WEST)->remove_neighbor(NEIGHBORS::SOUTH_EAST);
    center->get_neighbor(NEIGHBORS::SOUTH_WEST)->remove_neighbor(NEIGHBORS::NORTH_EAST);

    center->get_neighbor(NEIGHBORS::EAST)->init_inner_boundary();
    center->get_neighbor(NEIGHBORS::NORTH_EAST)->init_inner_boundary();
    center->get_neighbor(NEIGHBORS::SOUTH_EAST)->init_inner_boundary();
    center->get_neighbor(NEIGHBORS::WEST)->init_inner_boundary();
    center->get_neighbor(NEIGHBORS::NORTH_WEST)->init_inner_boundary();
    center->get_neighbor(NEIGHBORS::SOUTH_WEST)->init_inner_boundary();

    center->remove_neighbor(NEIGHBORS::WEST);
    center->remove_neighbor(NEIGHBORS::EAST);
    center->remove_neighbor(NEIGHBORS::NORTH_WEST);
    center->remove_neighbor(NEIGHBORS::SOUTH_WEST);
    center->remove_neighbor(NEIGHBORS::NORTH_EAST);
    center->remove_neighbor(NEIGHBORS::SOUTH_EAST);

    assert(center->get_type() == 0);

    for (auto& j : junctions_.container())
    {
        if (j.is_boundary())
        {
            rimguides_.push_back(j.get_rimguide());
        }
    }
}

void TriMesh::print_junction_types() const
{
    for (size_t y = 0; y < ly_; ++y)
    {
        const bool is_odd = y % 2 != 0;
        if (!is_odd)
        {
            std::cout << "  ";
        }
        for (size_t x = 0; x < lx_; ++x)
        {
            if (junctions_(x, y).get_type() == kInsideJunction)
            {
                std::cout << std::setw(3) << "A" << " ";
            }
            else
            {
                std::cout << std::setw(3) << static_cast<int>(junctions_(x, y).get_type()) << " ";
            }
        }
        std::cout << std::endl;
    }
}

void TriMesh::print_junction_pressure() const
{
    for (size_t y = 0; y < ly_; ++y)
    {
        const bool is_odd = y % 2 != 0;
        if (!is_odd)
        {
            std::cout << "   ";
        }
        for (size_t x = 0; x < lx_; ++x)
        {
            std::cout << std::setw(5) << std::fixed << std::setprecision(3) << (junctions_(x, y).get_output()) << " ";
        }
        std::cout << std::endl;
    }
}

void TriMesh::print_junction_pos() const
{
    for (size_t y = 0; y < ly_; ++y)
    {
        const bool is_odd = y % 2 != 0;
        if (is_odd)
        {
            std::cout << "      ";
        }
        for (size_t x = 0; x < lx_; ++x)
        {
            auto pos = junctions_(x, y).get_pos();
            std::cout << std::fixed << std::setprecision(2) << "(" << pos.x * 100 << "," << pos.y * 100 << ")     ";
        }
        std::cout << std::endl;
    }
}

void TriMesh::print_info() const
{
    for (const auto& j : junctions_.container())
    {
        j.print_info();
        std::cout << "----------------" << std::endl;
    }
}