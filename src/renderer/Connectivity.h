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

    std::array<int, 4> vertex_valences;
    std::array<const FaceData*, 8> one_ring{};
    std::array<int, 8> ring_rotation{};
    std::array<int, 16> control_points{};
    std::array<int, 25> subdivided_points{};

    bool regular;
    int inserted_vertex = -1;

    FaceData(const std::vector<int>& vertex_indices, const glm::vec3& face_normal, bool reg);

    int Valence() const { return vertices.size(); }
    int GetRingVertex(int face, int vertex) const {
        if ((face % 2) == 0 && one_ring[face] == nullptr) {
            return -1;
        } else {
            return one_ring[face]->vertices[(vertex + ring_rotation[face]) % 4];
        }
    }
};

using FaceDataPtr = std::unique_ptr<FaceData>;

struct EdgeData {
    const std::array<int, 2> vertices;
    std::array<FaceData*, 2> adjacent_faces;

    const int first_face_vertex;

    const float sharpness;
    int inserted_vertex = -1;

    EdgeData(int vertex1, int vertex2, FaceData* face1, FaceData* face2, int ffv, float sharp) noexcept;

    bool OnBoundary() const { return adjacent_faces[1] == nullptr; }
};

struct VertexData {
    std::vector<EdgeData*> adjacent_edges;
    std::unordered_set<FaceData*> adjacent_faces;
    std::vector<int> boundary_vertices;

    const int predecessor;
    const float sharpness;
    bool adjacent_irregular = false;
    int inserted_vertex = -1;

    VertexData(int pred, float sharp) noexcept;

    bool OnBoundary() const { return !boundary_vertices.empty(); }
    int Valence() const { return adjacent_edges.size(); }
    int FaceValence() const { return adjacent_faces.size(); }
};

std::vector<FaceDataPtr> GenerateFaceConnectivity(const std::vector<tinyobj::mesh_t>& meshes,
                                                  const std::vector<glm::vec3>& vertex_buffer);

std::vector<EdgeData> GenerateGlobalEdgeConnectivity(std::vector<FaceDataPtr>& face_data);
void FindFaceEdges(std::unordered_map<EdgeKey, EdgeData>& edges, FaceDataPtr& face, bool one_ring);

std::vector<VertexData> GenerateGlobalVertexConnectivity(std::vector<EdgeData>& edge_data);
std::vector<VertexData> GenerateIrregularVertexConnectivity(std::vector<EdgeData>& edge_data);
void FindEdgeVertices(std::unordered_map<int, VertexData>& vertices, EdgeData& edge);

void PopulateAdjacentOneRings(VertexData& vertex);
int IndexOfVertexInFace(const FaceData* face, const int vertex_index);
int RingFaceRotation(int face_index, int other_index);

void GenerateControlPoints(std::vector<FaceDataPtr>& face_data);

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
