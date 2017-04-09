#include "renderer/Subdivision.h"
#include "renderer/Mesh.h"

namespace Renderer {

IndexedMesh SubdivideMesh(const TinyObjMesh& obj) {
    // Initialize vertex buffer.
    std::vector<glm::vec3> vertex_buffer;
    for (std::size_t i = 0; i < obj.attrs.vertices.size(); i += 3) {
        vertex_buffer.emplace_back(obj.attrs.vertices[i], obj.attrs.vertices[i + 1], obj.attrs.vertices[i + 2]);
    }

    // Initialize faces.
    std::vector<FaceData> face_data{GenerateFaceConnectivity(obj.meshes, vertex_buffer)};

    int tess_level = 32;

    // Repeatedly subdivide the mesh until the tess level is satisfied.
    bool first_step = true;
    for (int t = tess_level; t > 1; t /= 2) {
        SubdivideFaces(face_data, vertex_buffer, first_step);
        first_step = false;
    }

    // Convert the face data into an index vector.
    std::vector<int> face_indices;
    for (const auto& face : face_data) {
        for (const auto& vertex_index : face.vertices) {
            face_indices.push_back(vertex_index);
        }
    }

    return {vertex_buffer, face_indices};
}

void SubdivideFaces(std::vector<FaceData>& face_data, std::vector<glm::vec3>& vertex_buffer, bool first_step) {
    std::vector<EdgeData> edge_data;
    std::vector<VertexData> vertex_data;
    if (first_step) {
        edge_data = GenerateGlobalEdgeConnectivity(face_data);
        vertex_data = GenerateGlobalVertexConnectivity(edge_data);
    } else {
        edge_data = GenerateIrregularEdgeConnectivity(face_data);
        vertex_data = GenerateIrregularVertexConnectivity(edge_data);
    }

    InsertVertices(vertex_buffer, face_data, edge_data, vertex_data);
    CreateNewFaces(vertex_buffer, face_data, vertex_data);
}

void InsertVertices(std::vector<glm::vec3>& vertex_buffer, std::vector<FaceData>& face_data,
                    std::vector<EdgeData>& edge_data, std::vector<VertexData>& vertex_data) {
    int new_vertex_index = vertex_buffer.size();

    for (auto& face : face_data) {
        if (!face.regular) {
            // Insert a new face vertex.
            glm::vec3 new_vertex(0.0f);
            for (const auto& vertex : face.vertices) {
                new_vertex += vertex_buffer[vertex];
            }

            vertex_buffer.push_back(new_vertex / static_cast<float>(face.Valence()));

            face.inserted_vertex = new_vertex_index++;
        }
    }

    for (auto& edge : edge_data) {
        if (edge.OnBoundary()) {
            glm::vec3 new_vertex{vertex_buffer[edge.vertices[0]] + vertex_buffer[edge.vertices[1]]};
            vertex_buffer.push_back(new_vertex / 2.0f);
        } else {
            for (auto& face : edge.adjacent_faces) {
                if (face->inserted_vertex == -1) {
                    // Face vertices aren't calculated for regular faces in the insertion step.
                    glm::vec3 inserted_face_vertex(0.0f);
                    for (const auto& vertex : face->vertices) {
                        inserted_face_vertex += vertex_buffer[vertex];
                    }

                    vertex_buffer.push_back(inserted_face_vertex / static_cast<float>(face->Valence()));

                    face->inserted_vertex = new_vertex_index++;
                }
            }

            glm::vec3 new_vertex{vertex_buffer[edge.vertices[0]] +
                                 vertex_buffer[edge.vertices[1]] +
                                 vertex_buffer[edge.adjacent_faces[0]->inserted_vertex] +
                                 vertex_buffer[edge.adjacent_faces[1]->inserted_vertex]};
            vertex_buffer.push_back(new_vertex / 4.0f);
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
                new_vertex += vertex_buffer[face->inserted_vertex];
            }

            float valence_f = vertex.Valence();
            new_vertex /= valence_f * valence_f;
            new_vertex += vertex_buffer[vertex.predecessor] * (valence_f - 2.0f) / valence_f;
        }

        vertex_buffer.push_back(new_vertex);

        vertex.inserted_vertex = new_vertex_index++;
    }
}

void CreateNewFaces(const std::vector<glm::vec3>& vertex_buffer,
                    std::vector<FaceData>& face_data,
                    const std::vector<VertexData>& vertex_data) {
    std::vector<FaceData> new_face_data;

    // Copy all regular faces. Irregular faces are replaced with their subdivided faces.
    for (const auto& face : face_data) {
        if (face.regular) {
            new_face_data.push_back(face);
            new_face_data.back().previously_irregular = false;
        }
    }

    for (const auto& vertex : vertex_data) {
        // After the control mesh vertex has been refined, we can add four new faces.
        // For each irregular face, iterate over it's edges to find the two adjacent to this vertex.
        for (const auto& face : vertex.adjacent_faces) {
            if (!face->regular) {
                std::vector<const EdgeData*> face_edges;
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
                glm::vec3 face_u{vertex_buffer[face_edges[0]->inserted_vertex] -
                                 vertex_buffer[face->inserted_vertex]};
                glm::vec3 face_v{vertex_buffer[face_edges[1]->inserted_vertex] -
                                 vertex_buffer[face->inserted_vertex]};
                glm::vec3 new_face_normal{glm::normalize(glm::cross(face_u, face_v))};

                if (glm::dot(face->normal, new_face_normal) > 0) {
                    std::vector<int> face_indices{face->inserted_vertex,
                                                  face_edges[0]->inserted_vertex,
                                                  vertex.inserted_vertex,
                                                  face_edges[1]->inserted_vertex};
                    new_face_data.emplace_back(face_indices, new_face_normal, vertex.Valence() == 4, true);
                } else {
                    std::vector<int> face_indices{face->inserted_vertex,
                                                  face_edges[1]->inserted_vertex,
                                                  vertex.inserted_vertex,
                                                  face_edges[0]->inserted_vertex};
                    new_face_data.emplace_back(face_indices, -new_face_normal, vertex.Valence() == 4, true);
                }
            }
        }
    }

    face_data.swap(new_face_data);
}

} // End namespace Renderer
