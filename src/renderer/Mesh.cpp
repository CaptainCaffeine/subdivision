#include <algorithm>
#include <stdexcept>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

#include "renderer/Mesh.h"

namespace Renderer {

TinyObjMesh::TinyObjMesh(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::mesh_t>& mesh)
        : attrs(attrib)
        , meshes(mesh) {}

IndexedMesh::IndexedMesh(const std::vector<glm::vec3>& verts, const std::vector<int>& indexes)
        : vertices(verts)
        , indices(indexes) {}

Material::Material(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec, float shine)
        : ambient(amb)
        , diffuse(diff)
        , specular(spec)
        , shininess(shine) {}

Mesh::Mesh(const std::vector<glm::vec3>& verts, const Material& material, const GLenum type)
        : vertices(verts)
        , mat(material)
        , primitive_type(type)
        , vbo(SetUpVBO(verts))
        , vao(SetUpVAO(vbo)) {}

Mesh::Mesh(const IndexedMesh& mesh, const Material& material, const GLenum type)
        : vertices(mesh.vertices)
        , indices(mesh.indices)
        , mat(material)
        , primitive_type(type)
        , vbo(SetUpVBO(vertices))
        , ebo(SetUpEBO(indices))
        , vao(SetUpVAO(vbo, ebo)) {}

GLuint Mesh::SetUpVBO(const std::vector<glm::vec3>& vertices) {
    GLuint vbo;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    return vbo;
}

GLuint Mesh::SetUpEBO(const std::vector<int>& indices) {
    GLuint ebo;
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    return ebo;
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

GLuint Mesh::SetUpVAO(const GLuint vbo, const GLuint ebo) {
    GLuint vao;
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

        // Position.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);
        glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    return vao;
}

void Mesh::DrawMesh(const GLuint shader_id, const glm::mat4& view_matrix) const {
    SetMaterial(shader_id);

    glBindVertexArray(vao);

    glUniformMatrix4fv(glGetUniformLocation(shader_id, "model"), 1, GL_FALSE, glm::value_ptr(model));

    GLint normal_mat_loc = glGetUniformLocation(shader_id, "normal_mat");
    glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(view_matrix * model)));
    glUniformMatrix3fv(normal_mat_loc, 1, GL_FALSE, glm::value_ptr(normal_matrix));

    if (ebo != 0) {
        glDrawElements(primitive_type, indices.size(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(primitive_type, 0, vertices.size());
    }
}

void Mesh::SetMaterial(const GLuint shader_id) const {
    glUniform3f(glGetUniformLocation(shader_id, "material.ambient"), mat.ambient.r, mat.ambient.g, mat.ambient.b);
    glUniform3f(glGetUniformLocation(shader_id, "material.diffuse"), mat.diffuse.r, mat.diffuse.g, mat.diffuse.b);
    glUniform3f(glGetUniformLocation(shader_id, "material.specular"), mat.specular.r, mat.specular.g, mat.specular.b);
    glUniform1f(glGetUniformLocation(shader_id, "material.shininess"), mat.shininess);
}

TinyObjMesh LoadTinyObjFromFile(const std::string& obj_filename) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    // I don't load materials right now, but this is still needed to call tinyobj::LoadObj.
    std::vector<tinyobj::material_t> materials;

    std::string err_msg;
    bool success = tinyobj::LoadObj(&attributes, &shapes, &materials, &err_msg, obj_filename.c_str(), nullptr, false);
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

    return {attributes, meshes};
}

std::vector<glm::vec3> PolygonSoup(const TinyObjMesh& tiny_obj) {
    std::vector<glm::vec3> mesh_data;

    // Usually only one mesh in an .obj file, but iterate over them just in case.
    for (const auto& mesh : tiny_obj.meshes) {
        // Iterate over each face in the mesh.
        std::size_t face_offset = 0;
        for (const auto& valence : mesh.num_face_vertices) {
            // Get the vertices and normals for each face from the provided indices.
            for(std::size_t v = 0; v < valence; ++v) {
                tinyobj::index_t idx = mesh.indices[face_offset + v];
                mesh_data.emplace_back(tiny_obj.attrs.vertices[3 * idx.vertex_index + 0],
                                       tiny_obj.attrs.vertices[3 * idx.vertex_index + 1],
                                       tiny_obj.attrs.vertices[3 * idx.vertex_index + 2]);
                mesh_data.emplace_back(tiny_obj.attrs.normals[3 * idx.normal_index + 0],
                                       tiny_obj.attrs.normals[3 * idx.normal_index + 1],
                                       tiny_obj.attrs.normals[3 * idx.normal_index + 2]);
            }

            face_offset += valence;
        }
    }

    return mesh_data;
}

} // End namespace Renderer
