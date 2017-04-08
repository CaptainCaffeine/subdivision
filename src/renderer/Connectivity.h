#pragma once

#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "externals/tiny_obj_loader.h"

namespace Renderer {

struct EdgeKey {
    int vertex1, vertex2;

    EdgeKey(int v1, int v2) noexcept;
};

constexpr bool operator==(const EdgeKey& lhs, const EdgeKey& rhs) noexcept {
    return (lhs.vertex1 == rhs.vertex1 && lhs.vertex2 == rhs.vertex2);
}

constexpr bool operator!=(const EdgeKey& lhs, const EdgeKey& rhs) noexcept {
    return !(lhs == rhs);
}

struct FaceData {
    std::vector<int> vertices;
    glm::vec3 normal;
    // Mutable so that we can mark faces adjacent to extraordinary vertices as irregular.
    // Some may argue this is a misuse of mutable, but I consider this better than
    //     a) having to const_cast the FaceData objects
    //     b) removing almost every instance of const in the connectivity code
    mutable bool regular;
    bool previously_irregular;
    mutable int inserted_vertex = -1;

    FaceData(std::vector<int> vertex_indices, const glm::vec3& face_normal, bool reg, bool prev_irreg);

    int Valence() const { return vertices.size(); }
};

struct EdgeData {
    std::array<int, 2> vertices;
    std::array<const FaceData*, 2> adjacent_faces;

    float sharpness;
    int inserted_vertex = -1;

    EdgeData(int vertex1, int vertex2, const FaceData* face1, const FaceData* face2, float sharp);

    bool OnBoundary() const { return adjacent_faces[1] == nullptr; }
};

struct VertexData {
    std::vector<const EdgeData*> adjacent_edges;
    std::unordered_set<const FaceData*> adjacent_faces;
    std::vector<int> boundary_vertices;

    int predecessor;
    float sharpness;
    int inserted_vertex = -1;

    VertexData(int pred, float sharp);

    bool OnBoundary() const { return !boundary_vertices.empty(); }
    int Valence() const { return adjacent_edges.size(); }
    int FaceValence() const { return adjacent_faces.size(); }
};

template<typename T, typename U>
using VectorPair = std::tuple<std::vector<T>, std::vector<U>>;

VectorPair<glm::vec3, int> SubdivideMesh(const tinyobj::attrib_t& attrib,
                                         const std::vector<tinyobj::mesh_t>& meshes);
void SubdivideFaces(std::vector<FaceData>& face_data, std::vector<glm::vec3>& vertex_buffer, bool first_step);
std::vector<FaceData> GenerateFaceConnectivity(const std::vector<tinyobj::mesh_t>& meshes,
                                               const std::vector<glm::vec3>& vertex_buffer);
std::vector<EdgeData> GenerateEdgeConnectivity(const std::vector<FaceData>& face_data);
std::vector<EdgeData> GenerateIrregularEdgeConnectivity(const std::vector<FaceData>& face_data);
std::vector<VertexData> GenerateVertexConnectivity(const std::vector<EdgeData>& edge_data);
std::vector<VertexData> GenerateIrregularVertexConnectivity(const std::vector<EdgeData>& edge_data);

// The data vectors are not const because the function needs to set the inserted vertex index.
void InsertVertices(std::vector<glm::vec3>& vertex_buffer, std::vector<FaceData>& face_data,
                    std::vector<EdgeData>& edge_data, std::vector<VertexData>& vertex_data);
void CreateNewFaces(const std::vector<glm::vec3>& vertex_buffer,
                    std::vector<FaceData>& face_data,
                    const std::vector<VertexData>& vertex_data);

} // End namespace Renderer

// Specialization of std::hash for EdgeKey.
namespace std {

template<>
struct hash<Renderer::EdgeKey> {
    typedef Renderer::EdgeKey argument_type;
    typedef std::size_t result_type;
    result_type operator()(const argument_type& e) const noexcept {
        const result_type r1{std::hash<int>{}(e.vertex1)};
        const result_type r2{std::hash<int>{}(e.vertex2)};
        return r1 ^ (r2 << 1);
    }
};

} // End namespace std
