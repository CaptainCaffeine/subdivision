#pragma once

#include <vector>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 

namespace Renderer {

struct DirLight {
    glm::vec3 direction, ambient, diffuse, specular;

    DirLight(const glm::vec3& dir, const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec)
            : direction(dir)
            , ambient(amb)
            , diffuse(diff)
            , specular(spec) {}
};

struct PointLight {
    glm::vec3 position, ambient, diffuse, specular;
    float constant, linear, quadratic;

    PointLight(const glm::vec3& pos, const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec,
               float constant_atten, float linear_atten, float quadratic_atten)
            : position(pos)
            , ambient(amb)
            , diffuse(diff)
            , specular(spec)
            , constant(constant_atten)
            , linear(linear_atten)
            , quadratic(quadratic_atten) {}
};

void RenderLoop(GLFWwindow* window, const std::vector<GLuint>& shaders, float win_width, float win_height);

GLuint CreateUBO(const std::size_t buffer_size, const GLenum access_type);
GLuint CreateSSBO(const std::size_t buffer_size, const GLenum access_type); // GL_DYNAMIC_COPY
GLuint SetMatricesUBO(float aspect);
GLuint SetTessellationUBO();
GLuint SetLightsUBO(bool dir_enable, bool point_enable,
                    const DirLight& dir_light, const std::vector<PointLight>& point_lights);

std::vector<glm::vec3> PatchVerts();

// Debug
void PrintUniformBlockOffsets(const GLuint shader_id, const std::string& block_name);

} // End namespace Renderer.
