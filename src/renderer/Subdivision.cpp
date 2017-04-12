#include <algorithm>
#include <cassert>
#include <iostream>

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
    std::vector<FaceDataPtr> face_data{GenerateFaceConnectivity(obj.meshes, vertex_buffer)};
    SubdivideFaces(face_data, vertex_buffer, 4);

    // Convert the face data into an index vector.
    std::vector<int> face_indices;
    for (const auto& face : face_data) {
        if (face->regular) {
            for (const auto& vertex_index : face->control_points) {
//                if (vertex_index < 0 || vertex_index >= vertex_buffer.size()) {
//                    std::cout << "not good\n";
//                }
                face_indices.push_back(vertex_index);
            }
        }
    }

    return {vertex_buffer, face_indices};
}

void SubdivideFaces(std::vector<FaceDataPtr>& face_data, std::vector<glm::vec3>& vertex_buffer, int tess_level) {
    std::vector<EdgeData> edge_data{GenerateGlobalEdgeConnectivity(face_data)};
    std::vector<VertexData> vertex_data{GenerateGlobalVertexConnectivity(edge_data)};
    GenerateControlPoints(face_data);

    for (int t = tess_level; t > 1; t /= 2) {
        CreateNewFaces(vertex_buffer, face_data, edge_data, vertex_data);
    }
}

void InsertFaceVertex(FaceData& face, std::vector<glm::vec3>& vertex_buffer) {
    if (face.inserted_vertex != -1) {
        // Don't generate a new vertex if we've already done so.
        return;
    }

    glm::vec3 new_vertex(0.0f);
    for (const auto& vertex : face.vertices) {
        new_vertex += vertex_buffer[vertex];
    }

    vertex_buffer.push_back(new_vertex / static_cast<float>(face.Valence()));

    face.inserted_vertex = vertex_buffer.size() - 1;
}

void InsertEdgeVertex(EdgeData& edge, std::vector<glm::vec3>& vertex_buffer) {
    if (edge.inserted_vertex != -1) {
        // Don't generate a new vertex if we've already done so.
        return;
    }

    if (edge.OnBoundary()) {
        glm::vec3 new_vertex{vertex_buffer[edge.vertices[0]] + vertex_buffer[edge.vertices[1]]};
        vertex_buffer.push_back(new_vertex / 2.0f);
    } else {
        for (const auto& face : edge.adjacent_faces) {
            InsertFaceVertex(*face, vertex_buffer);
        }

        glm::vec3 new_vertex{vertex_buffer[edge.vertices[0]] +
                             vertex_buffer[edge.vertices[1]] +
                             vertex_buffer[edge.adjacent_faces[0]->inserted_vertex] +
                             vertex_buffer[edge.adjacent_faces[1]->inserted_vertex]};
        vertex_buffer.push_back(new_vertex / 4.0f);
    }

    edge.inserted_vertex = vertex_buffer.size() - 1;
}

void RefineControlVertex(VertexData& vertex, std::vector<glm::vec3>& vertex_buffer) {
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
            if (edge->vertices[0] == vertex.predecessor) {
                new_vertex += vertex_buffer[edge->vertices[1]];
            } else {
                new_vertex += vertex_buffer[edge->vertices[0]];
            }
        }

        for (const auto& face : vertex.adjacent_faces) {
            new_vertex += vertex_buffer.at(face->inserted_vertex);
        }

        float valence_f = vertex.Valence();
        new_vertex /= valence_f * valence_f;
        new_vertex += vertex_buffer[vertex.predecessor] * (valence_f - 2.0f) / valence_f;
    }

    vertex_buffer.push_back(new_vertex);

    vertex.inserted_vertex = vertex_buffer.size() - 1;
}

void CreateNewFaces(std::vector<glm::vec3>& vertex_buffer,
                    std::vector<FaceDataPtr>& face_data,
                    std::vector<EdgeData>& edge_data,
                    std::vector<VertexData>& vertex_data) {
    SubdivideControlPoints(vertex_buffer, face_data);

    for (auto& vertex : vertex_data) {
        if (!vertex.adjacent_irregular) {
            continue;
        }

        // Calculate the inserted vertices for adjacent faces and edges before refining this vertex.

        for (auto& face : vertex.adjacent_faces) {
            InsertFaceVertex(*face, vertex_buffer);
        }

        for (auto& edge : vertex.adjacent_edges) {
            InsertEdgeVertex(*edge, vertex_buffer);
        }

        RefineControlVertex(vertex, vertex_buffer);
    }

    std::vector<FaceDataPtr> new_face_data;
    std::unordered_map<EdgeKey, EdgeData> edges;

    for (auto& vertex : vertex_data) {
        if (!vertex.adjacent_irregular) {
            continue;
        }

        // After the control mesh vertex has been refined, we can add four new faces.
        for (const auto& face : vertex.adjacent_faces) {
            if (!face->regular) {
                // For each irregular face, iterate over it's edges to find the two adjacent to the current vertex.
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
                glm::vec3 face_u{vertex_buffer.at(face_edges[0]->inserted_vertex) -
                                 vertex_buffer.at(face->inserted_vertex)};
                glm::vec3 face_v{vertex_buffer.at(face_edges[1]->inserted_vertex) -
                                 vertex_buffer.at(face->inserted_vertex)};
                glm::vec3 new_face_normal{glm::normalize(glm::cross(face_u, face_v))};

                if (glm::dot(face->normal, new_face_normal) > 0) {
                    std::vector<int> face_indices{face->inserted_vertex,
                                                  face_edges[0]->inserted_vertex,
                                                  vertex.inserted_vertex,
                                                  face_edges[1]->inserted_vertex};
                    new_face_data.push_back(std::make_unique<FaceData>(face_indices, new_face_normal,
                                                                       vertex.Valence() == 4));
                } else {
                    std::vector<int> face_indices{face->inserted_vertex,
                                                  face_edges[1]->inserted_vertex,
                                                  vertex.inserted_vertex,
                                                  face_edges[0]->inserted_vertex};
                    new_face_data.push_back(std::make_unique<FaceData>(face_indices, -new_face_normal,
                                                                       vertex.Valence() == 4));
                }

                // Find edges for the newly created face.
                FindFaceEdges(edges, new_face_data.back(), false);

                // Determine the corner of the parent face the new face is in.
                int row_offset, col_offset;
                std::tie(row_offset, col_offset) = SubpatchOffset(IndexOfVertexInFace(face, vertex.predecessor));

                for (int i = 0; i < 4; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        int point_index = face->subdivided_points[(i + row_offset) * 5 + (j + col_offset)];
                        new_face_data.back()->control_points[i * 4 + j] = point_index;
                    }
                }

            }
        }
    }

    // Copy all regular faces.
    for (auto& face : face_data) {
        if (face->regular) {
            new_face_data.push_back(std::move(face));
        }
    }

    // Replace the old mesh data.
    face_data = std::move(new_face_data);
    edge_data.clear();
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(edge_data),
                   [](const auto& e) { return e.second; });
    vertex_data = GenerateIrregularVertexConnectivity(edge_data);
}

void SubdivideControlPoints(std::vector<glm::vec3>& vertex_buffer, std::vector<FaceDataPtr>& face_data) {
    for (const auto& face : face_data) {
        if (!face->regular) {
            // Compute control point vertex positions for the subpatches.
            const auto stencil_weights{GetStencilWeights()};
            for (int i = 0; i < face->subdivided_points.size(); ++i) {
                glm::vec3 subdivided_vertex(0.0f);
                bool irregular = false;
                for (int j = 0; j < face->control_points.size(); ++j) {
                    //if (face->control_points[j] == -1 && stencil_weights[i][j] != 0.0f) {
                    //    irregular = true;
                    //    break;
                    //}

                    if (face->control_points[j] != -1) {
                        subdivided_vertex += stencil_weights[i][j] * vertex_buffer[face->control_points[j]];
                    }
                }

                if (!irregular) {
                    vertex_buffer.push_back(subdivided_vertex);
                    face->subdivided_points[i] = vertex_buffer.size() - 1;
                } else {
                    face->subdivided_points[i] = -1;
                }
            }
        }
    }
}

std::tuple<int, int> SubpatchOffset(int face_corner) {
    switch (face_corner) {
    case 0:
        return std::make_tuple(0, 0);
    case 1:
        return std::make_tuple(0, 1);
    case 2:
        return std::make_tuple(1, 1);
    case 3:
        return std::make_tuple(1, 0);
    default:
        throw std::runtime_error("Invalid value given for face vertex index: " + std::to_string(face_corner));
    }
}

std::array<std::array<float, 16>, 25> GetStencilWeights() {
    std::array<std::array<float, 16>, 25> stencil;

    stencil[0] = {{
        1.0f/4.0f, 1.0f/4.0f, 0, 0,
        1.0f/4.0f, 1.0f/4.0f, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    }};
    stencil[1] = {{
        1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f, 0,
        1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    }};
    stencil[2] = {{
        0, 1.0f/4.0f, 1.0f/4.0f, 0,
        0, 1.0f/4.0f, 1.0f/4.0f, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    }};
    stencil[3] = {{
        0, 1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f,
        0, 1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f,
        0, 0, 0, 0,
        0, 0, 0, 0
    }};
    stencil[4] = {{
        0, 0, 1.0f/4.0f, 1.0f/4.0f,
        0, 0, 1.0f/4.0f, 1.0f/4.0f,
        0, 0, 0, 0,
        0, 0, 0, 0
    }};

    stencil[5] = {{
        1.0f/16.0f, 1.0f/16.0f, 0, 0,
        3.0f/8.0f, 3.0f/8.0f, 0, 0,
        1.0f/16.0f, 1.0f/16.0f, 0, 0,
        0, 0, 0, 0
    }};
    stencil[6] = {{
        1.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f, 0,
        3.0f/32.0f, 9.0f/16.0f, 3.0f/32.0f, 0,
        1.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f, 0,
        0, 0, 0, 0
    }};
    stencil[7] = {{
        0, 1.0f/16.0f, 1.0f/16.0f, 0,
        0, 3.0f/8.0f, 3.0f/8.0f, 0,
        0, 1.0f/16.0f, 1.0f/16.0f, 0,
        0, 0, 0, 0
    }};
    stencil[8] = {{
        0, 1.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f,
        0, 3.0f/32.0f, 9.0f/16.0f, 3.0f/32.0f,
        0, 1.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f,
        0, 0, 0, 0
    }};
    stencil[9] = {{
        0, 0, 1.0f/16.0f, 1.0f/16.0f,
        0, 0, 3.0f/8.0f, 3.0f/8.0f,
        0, 0, 1.0f/16.0f, 1.0f/16.0f,
        0, 0, 0, 0
    }};

    stencil[10] = {{
        0, 0, 0, 0,
        1.0f/4.0f, 1.0f/4.0f, 0, 0,
        1.0f/4.0f, 1.0f/4.0f, 0, 0,
        0, 0, 0, 0
    }};
    stencil[11] = {{
        0, 0, 0, 0,
        1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f, 0,
        1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f, 0,
        0, 0, 0, 0
    }};
    stencil[12] = {{
        0, 0, 0, 0,
        0, 1.0f/4.0f, 1.0f/4.0f, 0,
        0, 1.0f/4.0f, 1.0f/4.0f, 0,
        0, 0, 0, 0
    }};
    stencil[13] = {{
        0, 0, 0, 0,
        0, 1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f,
        0, 1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f,
        0, 0, 0, 0
    }};
    stencil[14] = {{
        0, 0, 0, 0,
        0, 0, 1.0f/4.0f, 1.0f/4.0f,
        0, 0, 1.0f/4.0f, 1.0f/4.0f,
        0, 0, 0, 0
    }};

    stencil[15] = {{
        0, 0, 0, 0,
        1.0f/16.0f, 1.0f/16.0f, 0, 0,
        3.0f/8.0f, 3.0f/8.0f, 0, 0,
        1.0f/16.0f, 1.0f/16.0f, 0, 0
    }};
    stencil[16] = {{
        0, 0, 0, 0,
        1.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f, 0,
        3.0f/32.0f, 9.0f/16.0f, 3.0f/32.0f, 0,
        1.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f, 0
    }};
    stencil[17] = {{
        0, 0, 0, 0,
        0, 1.0f/16.0f, 1.0f/16.0f, 0,
        0, 3.0f/8.0f, 3.0f/8.0f, 0,
        0, 1.0f/16.0f, 1.0f/16.0f, 0
    }};
    stencil[18] = {{
        0, 0, 0, 0,
        0, 1.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f,
        0, 3.0f/32.0f, 9.0f/16.0f, 3.0f/32.0f,
        0, 1.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f
    }};
    stencil[19] = {{
        0, 0, 0, 0,
        0, 0, 1.0f/16.0f, 1.0f/16.0f,
        0, 0, 3.0f/8.0f, 3.0f/8.0f,
        0, 0, 1.0f/16.0f, 1.0f/16.0f
    }};

    stencil[20] = {{
        0, 0, 0, 0,
        0, 0, 0, 0,
        1.0f/4.0f, 1.0f/4.0f, 0, 0,
        1.0f/4.0f, 1.0f/4.0f, 0, 0
    }};
    stencil[21] = {{
        0, 0, 0, 0,
        0, 0, 0, 0,
        1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f, 0,
        1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f, 0
    }};
    stencil[22] = {{
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 1.0f/4.0f, 1.0f/4.0f, 0,
        0, 1.0f/4.0f, 1.0f/4.0f, 0
    }};
    stencil[23] = {{
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f,
        0, 1.0f/16.0f, 3.0f/8.0f, 1.0f/16.0f
    }};
    stencil[24] = {{
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 1.0f/4.0f, 1.0f/4.0f,
        0, 0, 1.0f/4.0f, 1.0f/4.0f
    }};

    return stencil;
}

} // End namespace Renderer
