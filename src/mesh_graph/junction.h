#pragma once

#include "vec2d.h"

#include <bitset>
#include <cstddef>
#include <memory>
#include <vector>

#include "rimguide.h"

enum NEIGHBORS
{
    NORTH_WEST = 0,
    NORTH_EAST = 1,
    EAST = 2,
    WEST = 3,
    SOUTH_WEST = 4,
    SOUTH_EAST = 5,
    NORTH = 0, // For the 4 port junction... there has to be a better way to do this
    SOUTH = 1,
};

enum JUNCTION_TYPE
{
    FOUR_PORT = 0,
    SIX_PORT = 1,
    UNDEFINED,
};

constexpr uint8_t kInsideJunction =
    (1 << NORTH_EAST) | (1 << EAST) | (1 << SOUTH_EAST) | (1 << SOUTH_WEST) | (1 << WEST) | (1 << NORTH_WEST);

/**
 * @brief A junction node in a triangular digital waveguide mesh
 *
 * Represents a node in a triangular mesh that can connect to up to 6 neighbors
 * in a hexagonal pattern (N-E, E, S-E, S-W, W, N-W directions).
 * Each junction processes wave scattering and propagation.
 */
class Junction
{
  public:
    Junction() = default;
    ~Junction();

    /** @brief Constructs a junction at the specified coordinates
     *  @param x X-coordinate of the junction
     *  @param y Y-coordinate of the junction  */
    void init(JUNCTION_TYPE type, float x, float y);

    Junction(const Junction& trijunction) = delete;
    Junction& operator=(const Junction& trijunction) = delete;
    Junction(Junction&& trijunction) noexcept;
    Junction& operator=(Junction&& trijunction) noexcept;

    /** @brief Clear the junction state */
    void clear();

    void reset();

    void set_absorption_coeff(float abs_coeff);

    void clamp(bool clamp);

    void add_neighbor(Junction* neighbor, NEIGHBORS dir);

    /** @brief Initialize the junction type based on neighbor connections */
    void init_junction_type();

    void init_boundary(const RimguideInfo& info);
    void init_inner_boundary();

    /** @brief Process wave scattering at this junction */
    void process_scatter();

    /** @brief Process wave propagation delays */
    void process_delay();

    void add_input(float input);

    float get_output() const;

    bool has_rimguide() const;
    const Rimguide* get_rimguide() const;
    Rimguide* get_rimguide();

    uint32_t get_type() const;
    Vec2Df get_pos() const;

    float get_energy() const;

    void print_info() const;

    Junction* get_neighbor(NEIGHBORS dir) const;
    void remove_neighbor(NEIGHBORS dir);

    bool is_boundary() const;

  private:
    void process_delay_four_port();
    void process_delay_six_port();

    void process_scatter_4port_1();
    void process_scatter_4port_2();

    void process_scatter_6port_1();
    void process_scatter_6port_2();

    JUNCTION_TYPE junction_type_ = JUNCTION_TYPE::UNDEFINED;
    std::bitset<6> type_ = 0;        // Type of junction (determined by number of connections)
    std::vector<float> in_ = {0.f};  // Incoming wave components
    std::vector<float> out_ = {0.f}; // Outgoing wave components

    Vec2Df pos_ = {0.f, 0.f}; // Junction position (x,y)

    float input_ = 0.f;                            // External input signal
    float pressure_ = 0.f;                         // Pressure value at the junction
    float abs_coeff_ = 0.f;                        // absorption coefficient at the junction
    std::vector<Junction*> neighbors_ = {nullptr}; // Connected neighbors
    size_t num_connection_ = 0;                    // Number of connected neighbors
    std::unique_ptr<Rimguide> rimguide_;           // Boundary condition (if this is a boundary node)

    bool use_alternate_ = false;
};