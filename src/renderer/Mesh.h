#pragma once

#include <vector>
#include <string>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "externals/tiny_obj_loader.h"
#include "renderer/Connectivity.h"

namespace Renderer {

struct Material;

class Mesh {
public:
    std::vector<glm::vec3> vertices;
    const Material& mat;
    const GLuint vbo, vao;
    glm::mat4 model;

    Mesh(const std::vector<glm::vec3>& vertices, const Material& material);

    void UpdateVBO();

    static std::vector<glm::vec3> LoadRegularMeshFromFile(const std::string& obj_filename);
    static void LoadControlMeshFromFile(const std::string& obj_filename);
private:
    using TinyObjMesh = std::tuple<tinyobj::attrib_t, std::vector<tinyobj::mesh_t>>;
    static TinyObjMesh LoadObjectFromFile(const std::string& obj_filename);

    static GLuint SetUpVBO(const std::vector<glm::vec3>& vertices);
    static GLuint SetUpVAO(const GLuint VBO);
};

} // End namespace Renderer.
