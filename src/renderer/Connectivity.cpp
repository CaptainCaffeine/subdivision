#include <unordered_map>
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

FaceData::FaceData(std::vector<int> vertex_indices, const glm::vec3& face_normal)
        : vertices(vertex_indices)
        , normal(face_normal)
        , regular((vertex_indices.size() == 4) ? true : false) {}

EdgeData::EdgeData(int vertex1, int vertex2, const FaceData* face1, const FaceData* face2, float sharp)
        : vertices({{vertex1, vertex2}})
        , adjacent_faces({{face1, face2}})
        , sharpness(sharp) {}

VertexData::VertexData(int pred, float sharp)
        : predecessor(pred)
        , sharpness(sharp) {}

VectorPair<glm::vec3, int> SubdivideMesh(const tinyobj::attrib_t& attrib,
                                         const std::vector<tinyobj::mesh_t>& meshes) {
    // Initialize vertex buffer.
    std::vector<glm::vec3> vertex_buffer;
    for (std::size_t i = 0; i < attrib.vertices.size(); i += 3) {
        vertex_buffer.emplace_back(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
    }

    // Initialize faces.
    std::vector<FaceData> face_data{GenerateFaceConnectivity(meshes, vertex_buffer)};

    int tess_level = 8;

    // Repeatedly subdivide the mesh until the tess level is satisfied.
    for (int t = tess_level; t > 1; t /= 2) {
        std::tie(vertex_buffer, face_data) = SubdivideFaces(face_data, vertex_buffer);
    }

    // Convert the face data into an index vector.
    std::vector<int> face_indices;
    for (const auto& face : face_data) {
        for (const auto& vertex_index : face.vertices) {
            face_indices.push_back(vertex_index);
        }
    }

    return std::make_tuple(vertex_buffer, face_indices);
}

VectorPair<glm::vec3, FaceData> SubdivideFaces(std::vector<FaceData>& face_data,
                                               const std::vector<glm::vec3>& vertex_buffer) {
    std::vector<EdgeData> edge_data = GenerateEdgeConnectivity(face_data);
    std::vector<VertexData> vertex_data = GenerateVertexConnectivity(edge_data);

    const std::vector<glm::vec3> new_vertex_buffer{InsertVertices(vertex_buffer, face_data, edge_data, vertex_data)};
    const std::vector<FaceData> new_face_data{CreateNewFaces(vertex_data, new_vertex_buffer)};

    return std::make_tuple(new_vertex_buffer, new_face_data);
}

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

            face_data.emplace_back(face_indices, glm::normalize(glm::cross(face_u, face_v)));
            face_offset += valence;
        }
    }

    return face_data;
}

std::vector<EdgeData> GenerateEdgeConnectivity(const std::vector<FaceData>& face_data) {
    // Iterate over all faces to obtain the edge connectivity information.
    // As most edges will be discovered twice, we keep track of generated edges in a map. We use a map instead
    // of a set, because we need to update the second face once the edge has already been inserted, and set elements
    // are immutable.
    std::unordered_map<EdgeKey, EdgeData> edges;

    // Iterate over all faces and find their edges.
    for (const auto& face : face_data) {
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

    // Transform the map values into a vector.
    std::vector<EdgeData> edge_data;
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(edge_data),
                   [](const auto& e) { return e.second; });

    return edge_data;
}

std::vector<VertexData> GenerateVertexConnectivity(const std::vector<EdgeData>& edge_data) {
    // Iterate over all edges to obtain the vertex connectivity information.
    std::unordered_map<int, VertexData> vertices;

    for (const auto& edge : edge_data) {
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

        if (edge.adjacent_faces[1] == nullptr) {
            map_vertex1.boundary_vertices.push_back(edge.vertices[1]);
            map_vertex2.boundary_vertices.push_back(edge.vertices[0]);
        } else {
            map_vertex1.adjacent_faces.insert(edge.adjacent_faces[1]);
            map_vertex2.adjacent_faces.insert(edge.adjacent_faces[1]);
        }
    }

    // Transform the map values into a vector.
    std::vector<VertexData> vertex_data;
    for (const auto& v : vertices) {
        if (v.second.boundary_vertices.size() > 2) {
            throw std::runtime_error("Found a vertex with " + std::to_string(v.second.boundary_vertices.size()) +
                                     " boundary edges. Non-manifold surfaces are not supported.");
        }

        vertex_data.emplace_back(v.second);
    }

    return vertex_data;
}

std::vector<glm::vec3> InsertVertices(const std::vector<glm::vec3>& vertex_buffer, std::vector<FaceData>& face_data,
                                      std::vector<EdgeData>& edge_data, std::vector<VertexData>& vertex_data) {
    std::vector<glm::vec3> new_vertex_buffer;
    int new_vertex_index = 0;

    for (auto& face : face_data) {
        glm::vec3 new_vertex(0.0f);
        for (const auto& vertex : face.vertices) {
            new_vertex += vertex_buffer[vertex];
        }

        new_vertex_buffer.push_back(new_vertex / static_cast<float>(face.Valence()));

        face.inserted_vertex = new_vertex_index++;
    }

    for (auto& edge : edge_data) {
        if (edge.OnBoundary()) {
            glm::vec3 new_vertex{vertex_buffer[edge.vertices[0]] + vertex_buffer[edge.vertices[1]]};
            new_vertex_buffer.push_back(new_vertex / 2.0f);
        } else {
            glm::vec3 new_vertex{vertex_buffer[edge.vertices[0]] +
                                 vertex_buffer[edge.vertices[1]] +
                                 new_vertex_buffer.at(edge.adjacent_faces[0]->inserted_vertex) +
                                 new_vertex_buffer.at(edge.adjacent_faces[1]->inserted_vertex)};
            new_vertex_buffer.push_back(new_vertex / 4.0f);
        }

        edge.inserted_vertex = new_vertex_index++;
    }

    for (auto& vertex : vertex_data) {
        glm::vec3 new_vertex(0.0f);

        if (vertex.OnBoundary()) {
            for (const auto& i : vertex.boundary_vertices) {
                new_vertex += vertex_buffer[i];
            }
            new_vertex += 6.0f * vertex_buffer[vertex.predecessor];

            new_vertex /= 8.0f;
        } else {
            for (const auto& edge : vertex.adjacent_edges) {
                // Add the vertex of the edge which is not the current vertex.
                // FIXME: This can be optimized to remove the branch by changing the predecessor weight, but I want
                // to get the algorithm right before I do that.
                if (edge->vertices[0] == vertex.predecessor) {
                    new_vertex += vertex_buffer[edge->vertices[1]];
                } else {
                    new_vertex += vertex_buffer[edge->vertices[0]];
                }
            }

            for (const auto& face : vertex.adjacent_faces) {
                new_vertex += new_vertex_buffer[face->inserted_vertex];
            }

            float valence_f = vertex.Valence();
            new_vertex /= valence_f * valence_f;
            new_vertex += vertex_buffer[vertex.predecessor] * (valence_f - 2.0f) / valence_f;
        }

        new_vertex_buffer.push_back(new_vertex);

        vertex.inserted_vertex = new_vertex_index++;
    }

    return new_vertex_buffer;
}

std::vector<FaceData> CreateNewFaces(const std::vector<VertexData>& vertex_data,
                                     const std::vector<glm::vec3>& new_vertex_buffer) {
    std::vector<FaceData> new_face_data;

    std::unordered_map<int, int> passthrough_vertices;

    for (auto& vertex : vertex_data) {
        // After the control mesh vertex has been refined, we can add four new faces.
        // For each irregular face, iterate over it's edges to find the two adjacent to this vertex.
        for (const auto& face : vertex.adjacent_faces) {
            std::vector<const EdgeData*> face_edges;
//            if (vertex.Valence() != 4 || !face->regular) {
                for (const auto& edge : vertex.adjacent_edges) {
                    if (edge->adjacent_faces[0] == face || edge->adjacent_faces[1] == face) {
                        face_edges.push_back(edge);
                    }
                }

                if (face_edges.size() != 2) {
                    throw std::runtime_error("Did not find two adjacent edges for face. Found " +
                                             std::to_string(face_edges.size()));
                }

                // Create the new face. Compare the cross product of the new edges with the normal of the face being
                // subdivided to ensure counterclockwise winding of vertices.
                glm::vec3 face_u{new_vertex_buffer[face_edges[0]->inserted_vertex] -
                                 new_vertex_buffer[face->inserted_vertex]};
                glm::vec3 face_v{new_vertex_buffer[face_edges[1]->inserted_vertex] -
                                 new_vertex_buffer[face->inserted_vertex]};
                glm::vec3 new_face_normal{glm::normalize(glm::cross(face_u, face_v))};

                if (glm::dot(face->normal, new_face_normal) > 0) {
                    std::vector<int> face_indices{face->inserted_vertex,
                                                  face_edges[0]->inserted_vertex,
                                                  vertex.inserted_vertex,
                                                  face_edges[1]->inserted_vertex};
                    new_face_data.emplace_back(face_indices, new_face_normal);
                } else {
                    std::vector<int> face_indices{face->inserted_vertex,
                                                  face_edges[1]->inserted_vertex,
                                                  vertex.inserted_vertex,
                                                  face_edges[0]->inserted_vertex};
                    new_face_data.emplace_back(face_indices, -new_face_normal);
                }
//            } else {
//                // Passthrough regular faces.
//            }
        }
    }

    return new_face_data;
}

} // End namespace Renderer
