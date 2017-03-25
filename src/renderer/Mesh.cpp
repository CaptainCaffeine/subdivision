#include <stdexcept>
#include <cstring>

#include "renderer/Mesh.h"

namespace Renderer {

Mesh::Mesh(const std::vector<glm::vec3>& verts, const Material& material)
        : vertices(verts)
        , mat(material)
        , VBO(SetUpVBO(verts))
        , VAO(SetUpVAO(VBO)) {}

GLuint Mesh::SetUpVBO(const std::vector<glm::vec3>& vertices) {
    GLuint VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

    return VBO;
}

GLuint Mesh::SetUpVAO(const GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Position.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // Normal.
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (GLvoid*)(sizeof(glm::vec3)));
        glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return VAO;
}

void Mesh::UpdateVBO() {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    GLfloat* buffer = static_cast<float*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
    if (!buffer) {
        throw std::runtime_error("glMapBuffer() returned a null pointer.");
    }

    std::memcpy(buffer, vertices.data(), vertices.size() * sizeof(glm::vec3));
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

//GLuint Mesh::SetUpBasicVAO(const GLuint VBO) {
//    GLuint VAO;
//    glGenVertexArrays(1, &VAO);
//
//    glBindVertexArray(VAO);
//
//        glBindBuffer(GL_ARRAY_BUFFER, VBO);
//
//        // Position.
//        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);
//        glEnableVertexAttribArray(0);
//
//    glBindVertexArray(0);
//
//    return VAO;
//}

} // End namespace Renderer.
