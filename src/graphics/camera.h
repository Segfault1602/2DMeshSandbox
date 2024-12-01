#pragma once

#include "glm/detail/qualifier.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include <glm/glm.hpp>

class Camera
{
  public:
    Camera();

    void reset(glm::vec3 position);

    void set_position(glm::vec3 position);
    void move(float deltaX, float deltaY);
    void add_zoom(float zoom);
    void rotate(float pitch, float yaw);

    glm::mat4 look_at();

  private:
    glm::vec3 position_{};

    float pitch_ = -0.10f;
    float yaw_ = 0.03f;
    float zoom_ = 1.f;

    float camX_ = 0.f;
    float camZ_ = 0.f;

    glm::vec3 up_{};
    glm::vec3 camera_right_{};
    glm::vec3 camera_up_{};
    glm::vec3 camera_target_{};
    glm::vec3 camera_direction_{};
    glm::vec3 camera_front_{};
};