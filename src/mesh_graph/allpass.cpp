#include "allpass.h"

#include <cstddef>

Allpass::Allpass(float a)
    : a_(a)
    , last_out_(0.0f)
    , delays_{0.0f, 0.0f}
{
}

void Allpass::setA(float a)
{
    a_ = a;
}

float Allpass::process(float x)
{
    float u_n = x - delays_[0];

    last_out_ = a_ * u_n + delays_[1];

    delays_[0] = u_n * a_;
    delays_[1] = u_n;

    return last_out_;
}

NonLinearAllpass::NonLinearAllpass(float a1, float a2)
    : a_{a1, a2}
    , last_out_(0.0f)
    , delays_{0.0f, 0.0f}
{
}

void NonLinearAllpass::setA(float a1, float a2)
{
    a_[0] = a1;
    a_[1] = a2;
}

float NonLinearAllpass::process(float x)
{
    float u_n = x - delays_[0];
    const size_t idx = (u_n > 0.0f) ? 0 : 1;

    last_out_ = a_[idx] * u_n + delays_[1];

    delays_[0] = u_n * a_[idx];
    delays_[1] = u_n;

    return last_out_;
}