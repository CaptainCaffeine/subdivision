#include <algorithm>
#include <iostream>

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

FaceData::FaceData(const std::vector<int>& vertex_indices, const glm::vec3& face_normal, bool reg)
        : vertices(vertex_indices)
        , normal(face_normal)
        , regular(reg) {}

EdgeData::EdgeData(int vertex1, int vertex2, FaceData* face1, FaceData* face2, int ffv, float sharp) noexcept
        : vertices({{vertex1, vertex2}})
        , adjacent_faces({{face1, face2}})
        , first_face_vertex(ffv)
        , sharpness(sharp) {}

VertexData::VertexData(int pred, float sharp) noexcept
        : predecessor(pred)
        , sharpness(sharp) {}

std::vector<FaceDataPtr> GenerateFaceConnectivity(const std::vector<tinyobj::mesh_t>& meshes,
                                                  const std::vector<glm::vec3>& vertex_buffer) {
    std::vector<FaceDataPtr> face_data;

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
            glm::vec3 face_normal{glm::normalize(glm::cross(face_u, face_v))};

            face_data.push_back(std::make_unique<FaceData>(face_indices, face_normal, valence == 4));
            face_offset += valence;
        }
    }

    return face_data;
}

std::vector<EdgeData> GenerateGlobalEdgeConnectivity(std::vector<FaceDataPtr>& face_data) {
    // As most edges will be discovered twice, we keep track of generated edges in a map. We use a map instead
    // of a set, because we need to update the second face once the edge has already been inserted, and set elements
    // are immutable.
    std::unordered_map<EdgeKey, EdgeData> edges;

    // Iterate over all faces and find their edges.
    for (auto& face : face_data) {
        FindFaceEdges(edges, face);
    }

    // Transform the map values into a vector.
    std::vector<EdgeData> edge_data;
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(edge_data),
                   [](const auto& e) { return e.second; });

    return edge_data;
}

void FindFaceEdges(std::unordered_map<EdgeKey, EdgeData>& edges, FaceDataPtr& face) {
    // Loop over each edge of the face.
    for (int v = 0; v < face->Valence(); ++v) {
        // The last edge is (valence - 1, 0), otherwise it's (v, v + 1).
        int first_index = face->vertices[v];
        int second_index = face->vertices[(v == face->Valence() - 1) ? 0 : v + 1];

        EdgeKey edge{first_index, second_index};
        auto map_insert = edges.emplace(edge, EdgeData{edge.vertex1, edge.vertex2, face.get(), nullptr, v, 0.0f});

        // Check if we have already found this edge.
        if (!map_insert.second) {
            EdgeData& map_edge = map_insert.first->second;
            if (map_edge.adjacent_faces[1] == nullptr) {
                // Edge has already been found once. Insert the offset for the second face.
                map_edge.adjacent_faces[1] = face.get();

                // Add the opposing faces to each other's one ring.
                FaceData* other_face = map_edge.adjacent_faces[0];
                if (face->one_ring[v * 2 + 1] != nullptr) {
                    throw std::runtime_error("Edge - Attempted to overwrite a face in a one ring.");
                }
                face->one_ring[v * 2 + 1] = other_face;

                if (other_face->one_ring[map_edge.first_face_vertex * 2 + 1] != nullptr) {
                    throw std::runtime_error("Edge - Attempted to overwrite a face in the other one ring.");
                }
                other_face->one_ring[map_edge.first_face_vertex * 2 + 1] = face.get();
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
    for (auto& v : vertices) {
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

        // Ignore irregular vertices for now.
        if (v.second.Valence() == 4) {
            PopulateAdjacentOneRings(v.second);
        }

        vertex_data.push_back(v.second);
    }

    for (auto& vertex : vertex_data) {
        // Mark this vertex if it is adjacent to any irregular faces.
        vertex.adjacent_irregular = (vertex.Valence() != 4) ||
                                    std::any_of(vertex.adjacent_faces.cbegin(), vertex.adjacent_faces.cend(),
                                                [](const FaceData* face) { return !face->regular; });
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
    for (auto& v : vertices) {
        // Only add this vertex if it is adjacent to an irregular face.
        v.second.adjacent_irregular = (v.second.Valence() != 4) ||
                                      std::any_of(v.second.adjacent_faces.cbegin(), v.second.adjacent_faces.cend(),
                                                  [](const FaceData* face) { return !face->regular; });
        if (v.second.adjacent_irregular) {
            if (v.second.boundary_vertices.size() > 2) {
                throw std::runtime_error("Found a vertex with " + std::to_string(v.second.boundary_vertices.size()) +
                                        " boundary edges. Non-manifold surfaces are not supported.");
            }

            // Ignore irregular vertices for now.
            if (v.second.Valence() == 4) {
                PopulateAdjacentOneRings(v.second);
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

void PopulateAdjacentOneRings(VertexData& vertex) {
    // For each adjacent face, check if it is present in the one ring of any other adjacent face. If it is not,
    // add it as a corner face.
    for (auto& face : vertex.adjacent_faces) {
        for (auto& other_face : vertex.adjacent_faces) {
            if (face == other_face) {
                continue;
            }

            auto found_face = std::find(other_face->one_ring.cbegin(), other_face->one_ring.cend(), face);
            if (found_face == other_face->one_ring.cend()) {
                // Not already present, so add each face to the other's one ring.
                for (int k = 0; k < face->Valence(); ++k) {
                    if (face->vertices[k] == vertex.predecessor) {
                        if (face->one_ring[k * 2] != nullptr) {
                            throw std::runtime_error("Vertex - Attempted to overwrite a face in a one ring.");
                        }

                        face->one_ring[k * 2] = other_face;
                        break;
                    }
                }

                for (int k = 0; k < other_face->Valence(); ++k) {
                    if (other_face->vertices[k] == vertex.predecessor) {
                        if (other_face->one_ring[k * 2] != nullptr) {
                            throw std::runtime_error("Vertex - Attempted to overwrite a face in the other one ring.");
                        }

                        other_face->one_ring[k * 2] = face;
                        break;
                    }
                }
            }
        }
    }
}

} // End namespace Renderer
