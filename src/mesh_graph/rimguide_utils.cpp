#include "rimguide_utils.h"

#include <algorithm>
#include <array>
#include <cmath>

Vec2Df get_boundary_position(float radius, const Vec2Df& junction_pos)
{
    float dist_2_center = get_distance({0, 0}, junction_pos);
    float x_unit = junction_pos.x / dist_2_center;
    float y_unit = junction_pos.y / dist_2_center;
    return {x_unit * radius, y_unit * radius};
}

float distance_between_point_and_line(Vec2Df P1, Vec2Df P2, Vec2Df P0)
{
    return std::abs((P2.y - P1.y) * P0.x - (P2.x - P1.x) * P0.y + P2.x * P1.y - P2.y * P1.x) /
           std::sqrt((P2.y - P1.y) * (P2.y - P1.y) + (P2.x - P1.x) * (P2.x - P1.x));
}

Vec2Df get_boundary_position_rect(float length, float width, const Vec2Df& junction_pos)
{
    std::array<float, 4> distances;
    // north
    distances[0] =
        distance_between_point_and_line({-length / 2.f, width / 2.f}, {length / 2.f, width / 2.f}, junction_pos);
    // south
    distances[1] =
        distance_between_point_and_line({-length / 2.f, -width / 2.f}, {length / 2.f, -width / 2.f}, junction_pos);
    // east
    distances[2] =
        distance_between_point_and_line({length / 2.f, -width / 2.f}, {length / 2.f, width / 2.f}, junction_pos);
    // west
    distances[3] =
        distance_between_point_and_line({-length / 2.f, -width / 2.f}, {-length / 2.f, width / 2.f}, junction_pos);

    float min_dist_idx = std::distance(distances.begin(), std::min_element(distances.begin(), distances.end()));

    switch (static_cast<int>(min_dist_idx))
    {
    case 0:
        return {junction_pos.x, width / 2.f};
    case 1:
        return {junction_pos.x, -width / 2.f};
    case 2:
        return {length / 2.f, junction_pos.y};
    case 3:
        return {-length / 2.f, junction_pos.y};
    default:
        return {0, 0};
    }
    return {0, 0};
}