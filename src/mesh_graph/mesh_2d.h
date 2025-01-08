#pragma once

#include "junction.h"
#include "mat2d.h"
#include "threadpool.h"
#include "vec2d.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class Rimguide;
struct RimguideInfo;

/**
 * @class Mesh2D
 * @brief Represents a 2D waveguide mesh
 */
class Mesh2D
{
  public:
    /**
     * @brief Constructs a Mesh2D object.
     */
    Mesh2D();

    /**
     * @brief Destroys the Mesh2D object.
     */
    virtual ~Mesh2D() = default;

    Mesh2D(const Mesh2D& mesh) = delete;
    Mesh2D& operator=(const Mesh2D& mesh) = delete;
    Mesh2D(Mesh2D&& mesh) = delete;
    Mesh2D& operator=(Mesh2D&& mesh) = delete;

    /**
     * @brief Clears the mesh.
     */
    virtual void clear();

    /**
     * @brief Gets a mask for a given radius.
     * @param radius The radius for which to get the mask.
     * @return A 2D matrix representing the mask.
     */
    virtual Mat2D<uint8_t> get_mask_for_radius(float radius) const;

    virtual Mat2D<uint8_t> get_mask_for_rect(float length, float width) const;

    /**
     * @brief Initializes the mesh with a given mask.
     * @param mask The mask to initialize the mesh with.
     * @note The mask is a matrix where 1 represents the inside of the mesh and 0 the outside.
     */
    virtual void init(const Mat2D<uint8_t>& mask) = 0;

    /**
     * @brief Initializes the boundary with given information.
     * @param info The information to initialize the boundary with.
     */
    virtual void init_boundary(const RimguideInfo& info);

    /**
     * @brief Clamps the center with a rimguide.
     * @note The center junction is removed from the mesh and a rimguide is added to all of its neighbors.
     */
    virtual void clamp_center_with_rimguide() = 0;

    /**
     * @brief Gets the sample rate.
     * @return The sample rate.
     */
    virtual size_t get_samplerate() const;

    /**
     * @brief Gets the energy present in the mesh.
     * @return The energy.
     * @note This does not include the energy in the rimguides.
     */
    virtual float get_energy() const;

    /**
     * @brief Sets the input position.
     * @param x The x-coordinate of the input. A value between 0 and 1.
     * @param y The y-coordinate of the input. A value between 0 and 1.
     */
    virtual void set_input(float x, float y);

    /**
     * @brief Sets the output position.
     * @param x The x-coordinate of the output. A value between 0 and 1.
     * @param y The y-coordinate of the output. A value between 0 and 1.
     */
    virtual void set_output(float x, float y);

    /**
     * @brief Sets the absorption coefficient.
     * @param coeff The absorption coefficient.
     * @note Currently unused.
     */
    virtual void set_absorption_coeff(float coeff);

    /**
     * @brief Processes a tick of the simulation with the given input.
     * @param input The input value.
     * @note This method will automatically use the appropriate number of threads.
     * @return The processed output value.
     */
    virtual float tick(float input);

    /**
     * @brief Processes a tick of the simulation with the given input using a single thread
     * @param input The input value.
     * @return The processed output value.
     */
    virtual float tick_st(float input);

    /**
     * @brief Processes a tick with the given input using multiple threads.
     * @param input The input value.
     * @return The processed output value.
     */
    virtual float tick_mt(float input);

    /**
     * @brief Gets the input position.
     * @return The input position as a 2D vector.
     */
    virtual Vec2Df get_input_pos() const;

    /**
     * @brief Gets the output position.
     * @return The output position as a 2D vector.
     */
    virtual Vec2Df get_output_pos() const;

    /**
     * @brief Gets the count of junctions.
     * @return The count of junctions.
     */
    virtual size_t get_junction_count() const;

    /**
     * @brief Gets the count of rimguides.
     * @return The count of rimguides.
     */
    virtual size_t get_rimguide_count() const;

    /**
     * @brief Gets a rimguide by index.
     * @param idx The index of the rimguide.
     * @return A pointer to the rimguide.
     */
    virtual Rimguide* get_rimguide(size_t idx);

    /**
     * @brief Prints the types of junctions.
     */
    virtual void print_junction_types() const = 0;

    /**
     * @brief Prints the pressure of junctions.
     */
    virtual void print_junction_pressure() const = 0;

    /**
     * @brief Gets the pressure of junctions.
     * @param pressure Pointer to store the pressure values.
     */
    virtual void get_junction_pressure(float* pressure) const;

    /**
     * @brief Gets the types of junctions.
     * @param types Pointer to store the types.
     */
    virtual void get_junction_types(uint8_t* types) const;

    size_t lx_{};
    size_t ly_{};
    Mat2D<Junction> junctions_;
    size_t input_x;
    size_t input_y;
    size_t output_x;
    size_t output_y;

    //  Non-owning pointers to rimguides for convenience
    std::vector<Rimguide*> rimguides_;

    ThreadPool threadpool_;
    size_t sample_rate_;

  private:
    /**
     * @brief Processes scatter in multiple threads.
     * @param start The start index.
     * @param end The end index.
     */
    void process_scatter_mt(size_t start, size_t end);

    /**
     * @brief Processes delay in multiple threads.
     * @param start The start index.
     * @param end The end index.
     */
    void process_delay_mt(size_t start, size_t end);
};