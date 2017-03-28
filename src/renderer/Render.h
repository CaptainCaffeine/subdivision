#pragma once

#include <vector>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 

namespace Renderer {

class Shader;
class Mesh;

struct Material {
    glm::vec3 ambient, diffuse, specular;
    float shininess;

    Material(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec, float shine)
            : ambient(amb)
            , diffuse(diff)
            , specular(spec)
            , shininess(shine) {}
};

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

void RenderLoop(GLFWwindow* window, const std::vector<Shader>& shaders, float win_width, float win_height);

void DrawMesh(const GLuint shader_id, const Mesh& mesh, const glm::mat4& view_matrix,
              const GLenum mesh_type = GL_TRIANGLES);

GLuint CreateUBO(std::size_t buffer_size);
GLuint SetMatricesUBO(const std::vector<Shader>& shaders, float aspect);
GLuint SetLightsUBO(const std::vector<Shader>& shaders, bool dir_enable, bool point_enable,
                    const DirLight& dir_light, const std::vector<PointLight>& point_lights);
void SetMaterial(const GLuint shader_id, const Material& mat);

std::vector<glm::vec3> CubeVertices();
std::vector<glm::vec3> CubeQuads();

// Debug
void PrintUniformBlockOffsets(const GLuint shader_id, const std::string& block_name);

} // End namespace Renderer.
