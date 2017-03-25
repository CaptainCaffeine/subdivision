#pragma once

#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace Renderer {

struct Material;

class Mesh {
public:
    std::vector<glm::vec3> vertices;
    const Material& mat;
    const GLuint VBO, VAO;
    glm::mat4 model;

    Mesh(const std::vector<glm::vec3>& vertices, const Material& material);

    static GLuint SetUpVBO(const std::vector<glm::vec3>& vertices);
    static GLuint SetUpVAO(const GLuint VBO);

    void UpdateVBO();

//    static GLuint SetUpBasicVAO(const GLuint VBO);
};

} // End namespace Renderer.
