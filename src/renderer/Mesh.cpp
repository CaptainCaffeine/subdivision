#include <stdexcept>
#include <iostream>
#include <cstring>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cpp
#include "externals/tiny_obj_loader.h"

#include "renderer/Mesh.h"

namespace Renderer {

Mesh::Mesh(const std::vector<glm::vec3>& verts, const Material& material)
        : vertices(verts)
        , mat(material)
        , vbo(SetUpVBO(verts))
        , vao(SetUpVAO(vbo)) {}

GLuint Mesh::SetUpVBO(const std::vector<glm::vec3>& vertices) {
    GLuint vbo;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

    return vbo;
}

GLuint Mesh::SetUpVAO(const GLuint vbo) {
    GLuint vao;
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Position.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // Normal.
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (GLvoid*)(sizeof(glm::vec3)));
        glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return vao;
}

void Mesh::UpdateVBO() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    GLfloat* buffer = static_cast<float*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
    if (!buffer) {
        throw std::runtime_error("glMapBuffer() returned a null pointer.");
    }

    std::memcpy(buffer, vertices.data(), vertices.size() * sizeof(glm::vec3));
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

std::vector<glm::vec3> Mesh::LoadObjectFromFile(const std::string& obj_filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    // I don't load materials right now, but this is still needed to call tinyobj::LoadObj.
    std::vector<tinyobj::material_t> materials;

    std::string err_msg;
    bool err = tinyobj::LoadObj(&attrib, &shapes, &materials, &err_msg, obj_filename.c_str());
    if (!err_msg.empty()) {
        std::cerr << err_msg << std::endl;
    }
    if (!err) {
        throw std::runtime_error("Error when attempting to load mesh from " + obj_filename);
    }

    std::vector<glm::vec3> mesh_data;

    // Usually only one shape in an .obj file.
    for (std::size_t s = 0; s < shapes.size(); ++s) {
        auto current_mesh = shapes[s].mesh;

        // Iterate over each face in the mesh.
        std::size_t face_offset = 0;
        for (std::size_t f = 0; f < current_mesh.num_face_vertices.size(); ++f) {
            std::size_t face_vertices = current_mesh.num_face_vertices[f];

            // Get the vertices and normals for each face from the provided indices.
            for(std::size_t v = 0; v < face_vertices; ++v) {
                tinyobj::index_t idx = current_mesh.indices[face_offset + v];
                mesh_data.emplace_back(attrib.vertices[3 * idx.vertex_index + 0],
                                       attrib.vertices[3 * idx.vertex_index + 1],
                                       attrib.vertices[3 * idx.vertex_index + 2]);
                mesh_data.emplace_back(attrib.normals[3 * idx.normal_index + 0],
                                       attrib.normals[3 * idx.normal_index + 1],
                                       attrib.normals[3 * idx.normal_index + 2]);
                // UV coordinates (currently unused).
                //float tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                //float ty = attrib.texcoords[2 * idx.texcoord_index + 1];
            }

            face_offset += face_vertices;
        }
    }

    return mesh_data;
}

} // End namespace Renderer.
