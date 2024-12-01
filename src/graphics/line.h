#pragma once

#include <glm/glm.hpp>
#include <vector>

class Line
{
  public:
    Line(std::vector<glm::vec3> start, std::vector<glm::vec3> end);
    ~Line();

    int set_mvp(glm::mat4 mvp);

    int set_color(glm::vec3 color);

    void update(std::vector<glm::vec3> start, std::vector<glm::vec3> end);

    int draw();

  private:
    // Vertex data structure
    struct Vertex
    {
        glm::vec3 position;
    };
    int shaderProgram;
    unsigned int VBO{}, VAO{};
    std::vector<Vertex> vertices;
    glm::vec3 startPoint{};
    glm::vec3 endPoint{};
    glm::mat4 MVP{};
    glm::vec3 lineColor{};
};