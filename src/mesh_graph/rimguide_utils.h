#pragma once

#include "vec2d.h"

Vec2Df get_boundary_position(float radius, const Vec2Df& junction_pos);
float distance_between_point_and_line(Vec2Df P1, Vec2Df P2, Vec2Df P0);
Vec2Df get_boundary_position_rect(float length, float width, const Vec2Df& junction_pos);