#pragma once

#include <cmath>
#include <cstdint>

template <typename T>
struct Vec3D
{
    T x;
    T y;
    T z;

    template <typename U>
    bool operator==(const Vec3D<U>& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

using Vec3Df = Vec3D<float>;
using Vec3Di = Vec3D<uint32_t>;

template <typename T>
inline float get_distance(const Vec3D<T>& a, const Vec3D<T>& b)
{
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}