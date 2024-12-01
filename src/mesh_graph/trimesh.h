#pragma once

#include "mesh_2d.h"

#include <cstddef>
#include <cstdint>

/**
 * @brief A triangular mesh representation in 2D space
 *
 * This class implements a triangular mesh structure that inherits from Mesh2D.
 * It provides functionality for creating and manipulating a 2D triangular mesh.
 */
class TriMesh : public Mesh2D
{
  public:
    /**
     * @brief Construct a new triangular mesh
     * @param lx Width of the mesh
     * @param ly Height of the mesh
     * @param sample_distance Distance between sample points
     */
    TriMesh(size_t lx, size_t ly, float sample_distance);

    /**
     * @brief Virtual destructor
     */
    ~TriMesh() override = default;

    TriMesh(const TriMesh& mesh) = delete;
    TriMesh& operator=(const TriMesh& mesh) = delete;
    TriMesh(TriMesh&& mesh) = delete;
    TriMesh& operator=(TriMesh&& mesh) = delete;

    /**
     * @brief Initialize the mesh using a binary mask
     * @param mask 2D binary mask defining the mesh structure
     */
    void init(const Mat2D<uint8_t>& mask) override;

    /**
     * @brief Clamp the center position using rim guidance
     */
    void clamp_center_with_rimguide() override;

    // Debugging functions

    /**
     * @brief Print the types of all junctions in the mesh
     */
    void print_junction_types() const override;

    /**
     * @brief Print the pressure values of all junctions
     */
    void print_junction_pressure() const override;

    /**
     * @brief Print the positions of all junctions
     */
    void print_junction_pos() const;

    /**
     * @brief Print general information about the mesh
     */
    void print_info() const;
};