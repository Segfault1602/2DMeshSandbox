#pragma once

#include "mesh_2d.h"

#include <cstddef>
#include <cstdint>

/**
 * @class RectilinearMesh
 * @brief A class representing a rectilinear 2D mesh graph.
 */
class RectilinearMesh : public Mesh2D
{
  public:
    /**
     * @brief Constructs a RectilinearMesh object.
     * @param lx The number of elements in the x-direction.
     * @param ly The number of elements in the y-direction.
     * @param sample_distance The distance between samples.
     */
    RectilinearMesh(size_t lx, size_t ly, float sample_distance);

    /**
     * @brief Destroys the RectilinearMesh object.
     */
    ~RectilinearMesh() override;

    RectilinearMesh(const RectilinearMesh& mesh) = delete;
    RectilinearMesh& operator=(const RectilinearMesh& mesh) = delete;
    RectilinearMesh(RectilinearMesh&& mesh) = delete;
    RectilinearMesh& operator=(RectilinearMesh&& mesh) = delete;

    /**
     * @brief Initializes the mesh graph with a given mask.
     * @param mask A 2D matrix representing the mask.
     */
    void init(const Mat2D<uint8_t>& mask) override;

    /**
     * @brief Clamps the center of the mesh graph using a rim guide.
     */
    void clamp_center_with_rimguide() override;

    /**
     * @brief Prints the types of junctions in the mesh graph.
     */
    void print_junction_types() const override;

    /**
     * @brief Prints the pressure at the junctions in the mesh graph.
     */
    void print_junction_pressure() const override;

  private:
    /**
     * @brief Performs a scattering pass on the mesh graph.
     * @param start The starting index for the pass.
     * @param stop The stopping index for the pass.
     */
    void scatter_pass(size_t start, size_t stop);

    /**
     * @brief Performs a delay pass on the mesh graph.
     * @param start The starting index for the pass.
     * @param stop The stopping index for the pass.
     */
    void delay_pass(size_t start, size_t stop);
};