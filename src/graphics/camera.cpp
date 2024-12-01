#include "camera.h"

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
constexpr glm::vec3 kCamUp = glm::vec3(0, 1, 0);
constexpr glm::vec3 kCamFront = glm::vec3(0, -1, -1);
} // namespace

Camera::Camera()
    : camera_front_(kCamFront)
{
    position_ = glm::vec3(0, 0, 0);
    up_ = glm::vec3(0, 1, 0);
}

void Camera::reset(glm::vec3 position)
{
}

void Camera::set_position(glm::vec3 position)
{
    this->position_ = position;

    camera_target_ = glm::vec3(0.0f, 0.0f, 0.0f);
    camera_direction_ = glm::normalize(position_ - camera_target_);

    camera_right_ = glm::normalize(glm::cross(up_, camera_direction_));
    camera_up_ = glm::cross(camera_direction_, camera_right_);
}

void Camera::move(float deltaX, float deltaY)
{
    constexpr float kSpeed = 0.1f;
    position_ += glm::vec3(0, 1, 0) * deltaY * kSpeed;
    position_ -= glm::normalize(glm::cross(kCamFront, camera_up_)) * deltaX * kSpeed;

    // camera_target_ += glm::vec3(0, 1, 0) * deltaY * kSpeed;
    // camera_target_ -= glm::normalize(glm::cross(kCamFront, camera_up_)) * deltaX * kSpeed;
}

void Camera::add_zoom(float zoom)
{
    constexpr float kSpeed = 0.1f;
    zoom_ += kSpeed * zoom;

    if (zoom_ < 0.1f)
        zoom_ = 0.1f;

    position_ = glm::normalize(position_) * zoom_;
}

void Camera::rotate(float pitch, float yaw)
{
    constexpr float kSensitivity = 1.f;

    pitch_ -= pitch * kSensitivity;
    yaw_ += yaw * kSensitivity;
    // position_ =
    //     glm::rotate(glm::mat4(1.0f), glm::radians(-pitch * kSensitivity), camera_right_) *
    //     glm::vec4(position_, 1.0f);

    // position_ = glm::rotate(glm::mat4(1.0f), glm::radians(yaw * kSensitivity), kCamFront) *
    // glm::vec4(position_, 1.0f);
}

glm::mat4 Camera::look_at()
{
    camX_ = zoom_ * sin(pitch_) * cos(yaw_);
    camZ_ = zoom_ * sin(pitch_) * sin(yaw_);
    float camY = zoom_ * cos(pitch_);

    glm::vec3 eye = glm::vec3(camX_, camZ_, camY);
    camera_direction_ = glm::normalize(camera_target_ - eye);
    glm::vec3 up = glm::normalize(glm::cross(camera_right_, camera_direction_));

    return glm::lookAt(eye, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
}