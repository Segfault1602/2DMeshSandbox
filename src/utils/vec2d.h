#pragma once

#include <cmath>
#include <cstdint>

template <typename T>
struct Vec2D
{
    T x;
    T y;

    template <typename U>
    bool operator==(const Vec2D<U>& other) const
    {
        return x == other.x && y == other.y;
    }
};

using Vec2Df = Vec2D<float>;
using Vec2Di = Vec2D<uint32_t>;

template <typename T>
inline float get_distance(const Vec2D<T>& a, const Vec2D<T>& b)
{
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}