#include "rectilinear_mesh.h"

#include "junction.h"
#include "mat2d.h"
#include "rimguide.h"
#include "vec2d.h"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#define IDX(x, y) ((x) + (y) * lx_)

RectilinearMesh::RectilinearMesh(size_t lx, size_t ly, float sample_distance)
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

    float x_offset = -floor(lx_ / 2.f);
    const float y_offset = -floor((ly_ / 2.f));

    for (size_t y = 0; y < ly_; ++y)
    {
        for (size_t x = 0; x < lx_; ++x)
        {
            float x_pos = x + x_offset;
            float y_pos = y + y_offset;

            // Scale the positions based on the sample distance
            x_pos *= sample_distance;
            y_pos *= sample_distance;

            junctions_(x, y).init(JUNCTION_TYPE::FOUR_PORT, x_pos, y_pos);
        }
    }
}

RectilinearMesh::~RectilinearMesh() = default;

void RectilinearMesh::init(const Mat2D<uint8_t>& mask)
{
    const size_t size = lx_ * ly_;

    if (mask.size() != size)
    {
        std::cerr << "Mask size does not match mesh size" << std::endl;
        return;
    }

    for (size_t y = 0; y < ly_; ++y)
    {
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

            if (y > 0 && x > 0 && mask(x, y - 1) == 1)
            {
                junctions_(x, y).add_neighbor(&junctions_(x, y - 1), SOUTH);
            }
            if (y < ly_ - 1 && mask(x, y + 1) == 1)
            {
                junctions_(x, y).add_neighbor(&junctions_(x, y + 1), NORTH);
            }
        }
    }

    for (auto& j : junctions_.container())
    {
        j.init_junction_type();
    }
}

void RectilinearMesh::clamp_center_with_rimguide()
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
    center->get_neighbor(NEIGHBORS::NORTH)->remove_neighbor(NEIGHBORS::SOUTH);
    center->get_neighbor(NEIGHBORS::SOUTH)->remove_neighbor(NEIGHBORS::NORTH);
    center->get_neighbor(NEIGHBORS::WEST)->remove_neighbor(NEIGHBORS::EAST);

    center->get_neighbor(NEIGHBORS::EAST)->init_inner_boundary();
    center->get_neighbor(NEIGHBORS::NORTH)->init_inner_boundary();
    center->get_neighbor(NEIGHBORS::SOUTH)->init_inner_boundary();
    center->get_neighbor(NEIGHBORS::WEST)->init_inner_boundary();

    center->remove_neighbor(NEIGHBORS::WEST);
    center->remove_neighbor(NEIGHBORS::EAST);
    center->remove_neighbor(NEIGHBORS::NORTH);
    center->remove_neighbor(NEIGHBORS::SOUTH);

    assert(center->get_type() == 0);
    for (auto& j : junctions_.container())
    {
        if (j.is_boundary())
        {
            rimguides_.push_back(j.get_rimguide());
        }
    }
}

void RectilinearMesh::print_junction_types() const
{
    for (size_t y = 0; y < ly_; ++y)
    {
        for (size_t x = 0; x < lx_; ++x)
        {
            size_t idx = x + (y * lx_);
            if (junctions_[idx].get_type() == 0b1111)
            {
                std::cout << std::setw(3) << "A" << " ";
            }
            else
            {
                std::cout << std::setw(3) << static_cast<int>(junctions_[idx].get_type()) << " ";
            }
        }
        std::cout << std::endl;
    }
}

void RectilinearMesh::print_junction_pressure() const
{
    for (size_t y = 0; y < ly_; ++y)
    {
        for (size_t x = 0; x < lx_; ++x)
        {
            size_t idx = x + (y * lx_);
            std::cout << std::setw(5) << std::fixed << std::setprecision(3) << (junctions_[idx].get_output() * 100)
                      << " ";
        }
        std::cout << std::endl;
    }
}
