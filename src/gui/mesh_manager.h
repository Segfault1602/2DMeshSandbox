#pragma once

#include <functional>
#include <glm/glm.hpp>

using RenderCompleteCallback = std::function<void()>;

class MeshManager
{
  public:
    MeshManager() = default;
    virtual ~MeshManager() = default;

    virtual void draw_config_menu(bool& reset_camera) = 0;
    virtual void draw_experimental_config_menu() = 0;
    virtual void render_async(float render_time_seconds, RenderCompleteCallback cb) = 0;

    virtual float get_progress() const = 0;
    virtual bool is_rendering() const = 0;
    virtual float get_render_runtime() const = 0;

    virtual void plot_mesh() const = 0;

    virtual void render_gl_mesh(glm::mat4 mvp) const = 0;
};