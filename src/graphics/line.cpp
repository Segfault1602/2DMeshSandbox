#include "line.h"

#include <cstddef>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Line::Line(std::vector<glm::vec3> start, std::vector<glm::vec3> end)
    : shaderProgram(glCreateProgram())
{
    lineColor = glm::vec3(1, 1, 1);
    MVP = glm::mat4(1.0f);

    const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec3 color;
uniform mat4 MVP;
out vec3 fragColor;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);

    // Color calculation based on Z coordinate
    if (aPos.z > 0.0) {
        float red = min(aPos.z, 0.1) / 0.1;
        fragColor = vec3(1.0, 1.0 - red, 1.0 - red);
    } else {
        float blue = min(-aPos.z, 0.1) / 0.1;
        fragColor = vec3(1.0 - blue, 1.0 - blue, 1.0);
    }
}
)";
    const char* fragmentShaderSource = R"(
#version 330 core
in vec3 fragColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(fragColor, 1.0);
}
)";

    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    // check for shader compile errors

    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    // check for shader compile errors

    // link shaders

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Update vertices array
    for (size_t i = 0; i < start.size(); i++)
    {
        Vertex v{};
        v.position = start[i];
        vertices.push_back(v);

        v.position = end[i];
        vertices.push_back(v);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Line::~Line()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
}

void Line::update(std::vector<glm::vec3> start, std::vector<glm::vec3> end)
{
    vertices.clear();
    for (size_t i = 0; i < start.size(); i++)
    {
        Vertex v{};
        v.position = start[i];
        vertices.push_back(v);

        v.position = end[i];
        vertices.push_back(v);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int Line::set_mvp(glm::mat4 mvp)
{
    MVP = mvp;
    return 1;
}

int Line::set_color(glm::vec3 color)
{
    lineColor = color;
    return 1;
}

int Line::draw()
{
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &lineColor[0]);

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, vertices.size());
    return 1;
}
