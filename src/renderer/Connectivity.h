#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "externals/tiny_obj_loader.h"

namespace Renderer {

struct Edge {
    int vertex1, vertex2;

    Edge(int v1, int v2) noexcept;
};

constexpr bool operator==(const Edge& lhs, const Edge& rhs) noexcept {
    return (lhs.vertex1 == rhs.vertex1 && lhs.vertex2 == rhs.vertex2);
}

constexpr bool operator!=(const Edge& lhs, const Edge& rhs) noexcept {
    return !(lhs == rhs);
}

struct Vertex {
    std::unordered_set<int> adj_vertices;
    std::unordered_set<int> adj_faces;
};

struct FaceData {
    int offset;
    int valence;
    int inserted_vertex;
    bool regular;

    FaceData(int off, int val, int inserted, bool reg);
};

struct EdgeData {
    int offset;
    int inserted_vertex;
    float sharpness;

    EdgeData(int off, int inserted, float sharp);
};

struct VertexData {
    int offset;
    int valence;
    int face_valence;
    int predecessor;
    float sharpness;

    VertexData(int off, int val, int fval, int pred, float sharp);
};

using EdgeMap = std::unordered_map<Edge, std::array<int, 2>>;
using VertexMap = std::unordered_map<int, Vertex>;
template<typename T>
using BufferPair = std::tuple<std::vector<int>, std::vector<T>>;

void GenerateMeshConnectivity(const std::vector<glm::vec3>& vertex_buffer, const std::vector<tinyobj::mesh_t>& meshes);

BufferPair<FaceData> PopulateFaceBuffers(const std::vector<tinyobj::mesh_t>& meshes, int inserted_vertex);
BufferPair<EdgeData> PopulateEdgeBuffers(const std::vector<int>& face_indices,
                                         const std::vector<FaceData>& face_data,
                                         int inserted_vertex);
BufferPair<VertexData> PopulateVertexBuffers(const std::vector<int>& edge_indices,
                                             const std::vector<EdgeData>& edge_data,
                                             int inserted_vertex);

EdgeMap GenerateEdgeConnectivity(const std::vector<int>& face_indices, const std::vector<FaceData>& face_data);
VertexMap GenerateVertexConnectivity(const std::vector<int>& edge_indices, const std::vector<EdgeData>& edge_data);

} // End namespace Renderer

// Specialization of std::hash for Edge.
namespace std {

template<>
struct hash<Renderer::Edge> {
    typedef Renderer::Edge argument_type;
    typedef std::size_t result_type;
    result_type operator()(const argument_type& e) const noexcept {
        const result_type r1{std::hash<int>{}(e.vertex1)};
        const result_type r2{std::hash<int>{}(e.vertex2)};
        return r1 ^ (r2 << 1);
    }
};

} // End namespace std
