#pragma once

#include <vector>
#include <string>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "externals/tiny_obj_loader.h"

namespace Renderer {

struct TinyObjMesh {
    tinyobj::attrib_t attrs;
    std::vector<tinyobj::mesh_t> meshes;

    TinyObjMesh(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::mesh_t>& mesh);
};

struct IndexedMesh {
    std::vector<glm::vec3> vertices;
    std::vector<int> indices;

    IndexedMesh(const std::vector<glm::vec3>& verts, const std::vector<int>& indexes);
};

struct Material {
    glm::vec3 ambient, diffuse, specular;
    float shininess;

    Material(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec, float shine);
};

TinyObjMesh LoadTinyObjFromFile(const std::string& obj_filename);
std::vector<glm::vec3> PolygonSoup(const TinyObjMesh& tiny_obj);

class Mesh {
public:
    std::vector<glm::vec3> vertices;
    std::vector<int> indices;
    const Material& mat;
    const GLenum primitive_type;
    const GLuint vbo, ebo = 0, vao;
    glm::mat4 model;

    Mesh(const std::vector<glm::vec3>& vertices, const Material& material, const GLenum type);
    Mesh(const IndexedMesh& mesh, const Material& material, const GLenum type);

    void DrawMesh(const GLuint shader_id, const glm::mat4& view_matrix) const;
private:
    void SetMaterial(const GLuint shader_id) const;

    static GLuint SetUpVBO(const std::vector<glm::vec3>& vertices);
    static GLuint SetUpEBO(const std::vector<int>& indices);
    static GLuint SetUpVAO(const GLuint vbo);
    static GLuint SetUpVAO(const GLuint vbo, const GLuint ebo);
};

} // End namespace Renderer.
