#include <algorithm>

#include "renderer/Connectivity.h"

namespace Renderer {

EdgeKey::EdgeKey(int v1, int v2) noexcept {
    // vertex1 always contains the smaller index.
    if (v1 < v2) {
        vertex1 = v1;
        vertex2 = v2;
    } else {
        vertex2 = v1;
        vertex1 = v2;
    }
}

FaceData::FaceData(std::vector<int> vertex_indices, const glm::vec3& face_normal, bool reg, bool prev_irreg)
        : vertices(vertex_indices)
        , normal(face_normal)
        , previously_irregular(prev_irreg)
        , regular(reg) {}

EdgeData::EdgeData(int vertex1, int vertex2, const FaceData* face1, const FaceData* face2, float sharp) noexcept
        : vertices({{vertex1, vertex2}})
        , adjacent_faces({{face1, face2}})
        , sharpness(sharp) {}

VertexData::VertexData(int pred, float sharp) noexcept
        : predecessor(pred)
        , sharpness(sharp) {}

std::vector<FaceData> GenerateFaceConnectivity(const std::vector<tinyobj::mesh_t>& meshes,
                                               const std::vector<glm::vec3>& vertex_buffer) {
    std::vector<FaceData> face_data;

    // Get the face-vertex data from the provided .obj.
    // Usually only one mesh in an .obj file, but iterate over them just in case.
    for (const auto& mesh : meshes) {
        int face_offset = 0;
        for (const auto& valence : mesh.num_face_vertices) {
            std::vector<int> face_indices;
            for (int v = 0; v < valence; ++v) {
                face_indices.push_back(mesh.indices[face_offset + v].vertex_index);
            }

            // We assume the face vertices come from the .obj file in counterclockwise order, and so we calculate
            // the face normal here to orient subdivided faces later.
            glm::vec3 face_u{vertex_buffer[face_indices[1]] - vertex_buffer[face_indices[0]]};
            glm::vec3 face_v{vertex_buffer[face_indices[2]] - vertex_buffer[face_indices[0]]};

            face_data.emplace_back(face_indices, glm::normalize(glm::cross(face_u, face_v)), valence == 4, false);
            face_offset += valence;
        }
    }

    return face_data;
}

std::vector<EdgeData> GenerateGlobalEdgeConnectivity(const std::vector<FaceData>& face_data) {
    // As most edges will be discovered twice, we keep track of generated edges in a map. We use a map instead
    // of a set, because we need to update the second face once the edge has already been inserted, and set elements
    // are immutable.
    std::unordered_map<EdgeKey, EdgeData> edges;

    // Iterate over all faces and find their edges.
    for (const auto& face : face_data) {
        FindFaceEdges(edges, face);
    }

    // Transform the map values into a vector.
    std::vector<EdgeData> edge_data;
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(edge_data),
                   [](const auto& e) { return e.second; });

    return edge_data;
}

std::vector<EdgeData> GenerateIrregularEdgeConnectivity(const std::vector<FaceData>& face_data) {
    // As most edges will be discovered twice, we keep track of generated edges in a map. We use a map instead
    // of a set, because we need to update the second face once the edge has already been inserted, and set elements
    // are immutable.
    std::unordered_map<EdgeKey, EdgeData> edges;

    // Iterate over all irregular and previously irregular faces, and add their edges.
    for (const auto& face : face_data) {
        if (!face.regular || face.previously_irregular) {
            FindFaceEdges(edges, face);
        }
    }

    // Transform the map values into a vector.
    std::vector<EdgeData> edge_data;
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(edge_data),
                   [](const auto& e) { return e.second; });

    return edge_data;
}

void FindFaceEdges(std::unordered_map<EdgeKey, EdgeData>& edges, const FaceData& face) {
    // Loop over each edge of the face.
    for (int v = 0; v < face.Valence(); ++v) {
        // The last edge is (valence - 1, 0), otherwise it's (v, v + 1).
        int first_index = face.vertices[v];
        int second_index = face.vertices[(v == face.Valence() - 1) ? 0 : v + 1];

        EdgeKey edge{first_index, second_index};
        auto map_insert = edges.emplace(edge, EdgeData{edge.vertex1, edge.vertex2, &face, nullptr, 0.0f});

        // Check if we have already found this edge.
        if (!map_insert.second) {
            if (map_insert.first->second.adjacent_faces[1] == nullptr) {
                // Edge has already been found once. Insert the offset for the second face.
                map_insert.first->second.adjacent_faces[1] = &face;
            } else {
                // Edge found a third time - we do not handle meshes with edges adjacent to more than 2 faces.
                throw std::runtime_error("Edge with valence > 2 found in mesh.");
            }
        }
    }
}

std::vector<VertexData> GenerateGlobalVertexConnectivity(const std::vector<EdgeData>& edge_data) {
    // Iterate over all edges to obtain the vertex connectivity information.
    std::unordered_map<int, VertexData> vertices;

    for (const auto& edge : edge_data) {
        FindEdgeVertices(vertices, edge);
    }

    // Transform the map values into a vector.
    std::vector<VertexData> vertex_data;
    for (const auto& v : vertices) {
        if (v.second.boundary_vertices.size() > 2) {
            throw std::runtime_error("Found a vertex with " + std::to_string(v.second.boundary_vertices.size()) +
                                     " boundary edges. Non-manifold surfaces are not supported.");
        }

        // If this is an extraordinary vertex, mark adjacent faces as irregular.
        if (v.second.Valence() != 4) {
            for (auto& face : v.second.adjacent_faces) {
                face->regular = false;
            }
        }

        vertex_data.push_back(v.second);
    }

    return vertex_data;
}

std::vector<VertexData> GenerateIrregularVertexConnectivity(const std::vector<EdgeData>& edge_data) {
    // Iterate over all edges to obtain the vertex connectivity information.
    std::unordered_map<int, VertexData> vertices;

    for (const auto& edge : edge_data) {
        FindEdgeVertices(vertices, edge);
    }

    // Transform the map values into a vector.
    std::vector<VertexData> vertex_data;
    for (const auto& v : vertices) {
        // Only add this vertex if it is adjacent to an irregular face.
        bool irregular = std::any_of(v.second.adjacent_faces.begin(), v.second.adjacent_faces.end(),
                                     [](const FaceData* face) { return !face->regular; });
        if (irregular) {
            if (v.second.boundary_vertices.size() > 2) {
                throw std::runtime_error("Found a vertex with " + std::to_string(v.second.boundary_vertices.size()) +
                                        " boundary edges. Non-manifold surfaces are not supported.");
            }

            vertex_data.push_back(v.second);
        }
    }

    return vertex_data;
}

void FindEdgeVertices(std::unordered_map<int, VertexData>& vertices, const EdgeData& edge) {
    // Attempt to emplace the vertices into the map, the existing element will be returned if we've found
    // this vertex already.
    auto map_insert1 = vertices.emplace(edge.vertices[0], VertexData(edge.vertices[0], 0.0f));
    auto map_insert2 = vertices.emplace(edge.vertices[1], VertexData(edge.vertices[1], 0.0f));

    VertexData& map_vertex1 = map_insert1.first->second;
    VertexData& map_vertex2 = map_insert2.first->second;

    // Insert the neighbouring vertices of each vertex from this edge.
    map_vertex1.adjacent_edges.push_back(&edge);
    map_vertex1.adjacent_faces.insert(edge.adjacent_faces[0]);

    map_vertex2.adjacent_edges.push_back(&edge);
    map_vertex2.adjacent_faces.insert(edge.adjacent_faces[0]);

    if (edge.OnBoundary()) {
        map_vertex1.boundary_vertices.push_back(edge.vertices[1]);
        map_vertex2.boundary_vertices.push_back(edge.vertices[0]);
    } else {
        map_vertex1.adjacent_faces.insert(edge.adjacent_faces[1]);
        map_vertex2.adjacent_faces.insert(edge.adjacent_faces[1]);
    }
}

} // End namespace Renderer
