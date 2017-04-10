#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <memory>

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
    const std::vector<int> vertices;
    const glm::vec3 normal;
    std::array<const FaceData*, 8> one_ring{};
    // Mutable so that we can mark faces adjacent to extraordinary vertices as irregular.
    // Some may argue this is a misuse of mutable, but I consider this better than
    //     a) having to const_cast the FaceData objects
    //     b) removing almost every instance of const in the connectivity code
    mutable bool regular;
    mutable int inserted_vertex = -1;

    FaceData(const std::vector<int>& vertex_indices, const glm::vec3& face_normal, bool reg);

    int Valence() const { return vertices.size(); }
};

using FaceDataPtr = std::unique_ptr<FaceData>;

struct EdgeData {
    const std::array<int, 2> vertices;
    std::array<FaceData*, 2> adjacent_faces;

    const int first_face_vertex;
    const float sharpness;
    mutable int inserted_vertex = -1;

    EdgeData(int vertex1, int vertex2, FaceData* face1, FaceData* face2, int ffv, float sharp) noexcept;

    bool OnBoundary() const { return adjacent_faces[1] == nullptr; }
};

struct VertexData {
    std::vector<const EdgeData*> adjacent_edges;
    std::unordered_set<FaceData*> adjacent_faces;
    std::vector<int> boundary_vertices;

    const int predecessor;
    const float sharpness;
    bool adjacent_irregular = false;
    mutable int inserted_vertex = -1;

    VertexData(int pred, float sharp) noexcept;

    bool OnBoundary() const { return !boundary_vertices.empty(); }
    int Valence() const { return adjacent_edges.size(); }
    int FaceValence() const { return adjacent_faces.size(); }
};

std::vector<FaceDataPtr> GenerateFaceConnectivity(const std::vector<tinyobj::mesh_t>& meshes,
                                                  const std::vector<glm::vec3>& vertex_buffer);

std::vector<EdgeData> GenerateGlobalEdgeConnectivity(std::vector<FaceDataPtr>& face_data);
void FindFaceEdges(std::unordered_map<EdgeKey, EdgeData>& edges, FaceDataPtr& face);

std::vector<VertexData> GenerateGlobalVertexConnectivity(const std::vector<EdgeData>& edge_data);
std::vector<VertexData> GenerateIrregularVertexConnectivity(const std::vector<EdgeData>& edge_data);
void FindEdgeVertices(std::unordered_map<int, VertexData>& vertices, const EdgeData& edge);
void PopulateAdjacentOneRings(VertexData& vertex);

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
