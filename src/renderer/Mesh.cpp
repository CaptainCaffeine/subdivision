#include <tuple>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cstring>

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

Mesh::TinyObjMesh Mesh::LoadObjectFromFile(const std::string& obj_filename) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    // I don't load materials right now, but this is still needed to call tinyobj::LoadObj.
    std::vector<tinyobj::material_t> materials;

    std::string err_msg;
    bool success = tinyobj::LoadObj(&attributes, &shapes, &materials, &err_msg, obj_filename.c_str());
    if (!err_msg.empty()) {
        std::cerr << err_msg << std::endl;
    }
    if (!success) {
        throw std::runtime_error("Error when attempting to load mesh from " + obj_filename);
    }

    // Haven't found any use for the name field in the shape_t struct, so I just grab the mesh_t's.
    std::vector<tinyobj::mesh_t> meshes;
    std::transform(shapes.cbegin(), shapes.cend(), std::back_inserter(meshes),
                   [](const tinyobj::shape_t& shape) { return shape.mesh; });

    return std::make_tuple(attributes, meshes);
}

std::vector<glm::vec3> Mesh::LoadRegularMeshFromFile(const std::string& obj_filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::mesh_t> meshes;
    std::tie(attrib, meshes) = LoadObjectFromFile(obj_filename);

    std::vector<glm::vec3> mesh_data;

    // Usually only one mesh in an .obj file, but iterate over them just in case.
    for (const auto& mesh : meshes) {
        // Iterate over each face in the mesh.
        std::size_t face_offset = 0;
        for (const auto& valence : mesh.num_face_vertices) {

            // Get the vertices and normals for each face from the provided indices.
            for(std::size_t v = 0; v < valence; ++v) {
                tinyobj::index_t idx = mesh.indices[face_offset + v];
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

            face_offset += valence;
        }
    }

    return mesh_data;
}

void Mesh::LoadControlMeshFromFile(const std::string& obj_filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::mesh_t> meshes;
    std::tie(attrib, meshes) = LoadObjectFromFile(obj_filename);

    // Initialize vertex buffer.
    std::vector<glm::vec3> vertex_buffer;
    for (std::size_t i = 0; i < attrib.vertices.size(); i += 3) {
        vertex_buffer.emplace_back(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
    }

    GenerateMeshConnectivity(vertex_buffer, meshes);
}

} // End namespace Renderer
