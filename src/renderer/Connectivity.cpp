#include <algorithm>

#include "renderer/Connectivity.h"

namespace Renderer {

Edge::Edge(int v1, int v2) noexcept {
    // vertex1 always contains the smaller index.
    if (v1 < v2) {
        vertex1 = v1;
        vertex2 = v2;
    } else {
        vertex2 = v1;
        vertex1 = v2;
    }
}

FaceData::FaceData(int off, int val, int inserted, bool reg)
        : offset(off)
        , valence(val)
        , inserted_vertex(inserted)
        , regular(reg) {}

EdgeData::EdgeData(int off, int inserted, float sharp)
        : offset(off)
        , inserted_vertex(inserted)
        , sharpness(sharp) {}

VertexData::VertexData(int off, int val, int fval, int pred, float sharp)
        : offset(off)
        , valence(val)
        , face_valence(fval)
        , predecessor(pred)
        , sharpness(sharp) {}

void GenerateMeshConnectivity(const std::vector<glm::vec3>& vertex_buffer,
                              const std::vector<tinyobj::mesh_t>& meshes) {
    int inserted_vertex = vertex_buffer.size();

    // Contains indices into the vertex_buffer.
    std::vector<int> face_indices;
    // Contains offsets into face_indices, valences, the index of the inserted vertex, and regularity for each face.
    std::vector<FaceData> face_data;
    std::tie(face_indices, face_data) = PopulateFaceBuffers(meshes, inserted_vertex);
    inserted_vertex += face_data.size();

    // Contains indices for the two adjacent control vertices, and the adjacent inserted face vertices.
    std::vector<int> edge_indices;
    // Contains offsets into edge_indices, the index of the inserted edge index, and the sharpness value.
    std::vector<EdgeData> edge_data;
    std::tie(edge_indices, edge_data) = PopulateEdgeBuffers(face_indices, face_data, inserted_vertex);
    inserted_vertex += edge_data.size();

    // Contains indices to the adjacent control mesh vertices and inserted face vertices.
    std::vector<int> vertex_indices;
    // Contains offsets into vertex_indices, edge valences, face valences, an index to the vertex being refined,
    // and the sharpness value.
    std::vector<VertexData> vertex_data;
    std::tie(vertex_indices, vertex_data) = PopulateVertexBuffers(edge_indices, edge_data, inserted_vertex);

    // Iterate over vertices and mark irregular faces.
    for (const auto& vertex : vertex_data) {
        if (vertex.valence != 4) {
            for (int j = 0; j < vertex.face_valence; ++j) {
                // Set the regular flag to zero (false);
                face_data[vertex_indices[vertex.offset + vertex.valence + j]].regular = false;
            }
        }
    }


}

BufferPair<FaceData> PopulateFaceBuffers(const std::vector<tinyobj::mesh_t>& meshes, int inserted_vertex) {
    // Contains indices into the vertex_buffer.
    std::vector<int> face_indices;
    // Contains offsets into face_indices, valences, and the index of the inserted vertex for each face.
    std::vector<FaceData> face_data;

    // Get the face-vertex data from the provided .obj.
    // Usually only one shape in an .obj file, but iterate over them just in case.
    for (const auto& mesh : meshes) {
        // Face indices.
        std::transform(mesh.indices.cbegin(), mesh.indices.cend(), std::back_inserter(face_indices),
                       [](const tinyobj::index_t& index) { return index.vertex_index; });

        // Face data.
        int face_offset = 0;
        for (const auto& valence : mesh.num_face_vertices) {
            face_data.emplace_back(face_offset, valence, inserted_vertex++, (valence == 4) ? 1 : 0);
            face_offset += valence;

            // Generate the new face vertex.
            //glm::vec3 new_vertex(0.0f);
            //for (int v = 0; v < valence; ++v) {
            //    new_vertex += vertex_buffer[face_indices[face_offset + v]];
            //}

            //vertex_buffer.push_back(new_vertex);
        }
    }

    return std::make_tuple(face_indices, face_data);
}

BufferPair<EdgeData> PopulateEdgeBuffers(const std::vector<int>& face_indices,
                                         const std::vector<FaceData>& face_data,
                                         int inserted_vertex) {
    // Contains indices for the two adjacent control vertices, and the adjacent inserted face vertices.
    std::vector<int> edge_indices;
    // Contains offsets into edge_indices, the index of the inserted edge index, and the sharpness value.
    std::vector<EdgeData> edge_data;

    const auto edge_map{GenerateEdgeConnectivity(face_indices, face_data)};

    // Keep value of inserted_vertex from above.
    int edge_offset = 0;
    for (const auto& e : edge_map) {
        // Offsets into vertex_buffer.
        edge_indices.emplace_back(e.first.vertex1);
        edge_indices.emplace_back(e.first.vertex2);
        // Offsets into face_data.
        edge_indices.emplace_back(e.second[0]);
        edge_indices.emplace_back(e.second[1]);

        edge_data.emplace_back(edge_offset, inserted_vertex++, 0.0f);
        edge_offset += 4;

        // Edge vertex insertion.
        //bool boundary = e.second[1] == -1;
        //if (boundary) {
        //    glm::vec3 new_vertex{vertex_buffer[e.first.vertex1] + vertex_buffer[e.first.vertex2]};
        //    vertex_buffer.push_back(new_vertex / 2.0f);
        //} else {
        //    glm::vec3 new_vertex{vertex_buffer[e.first.vertex1] +
        //                        vertex_buffer[e.first.vertex2] +
        //                        vertex_buffer[face_data[e.second[0] + 2]] +
        //                        vertex_buffer[face_data[e.second[1] + 2]]};
        //    vertex_buffer.push_back(new_vertex / 4.0f);
        //}
    }

    return std::make_tuple(edge_indices, edge_data);
}

BufferPair<VertexData> PopulateVertexBuffers(const std::vector<int>& edge_indices,
                                             const std::vector<EdgeData>& edge_data,
                                             int inserted_vertex) {
    // Contains indices to the adjacent control mesh vertices and inserted face vertices.
    // How about an offset into face data? So an irregular flag could be set
    std::vector<int> vertex_indices;
    // Contains offsets into vertex_indices, edge valences, face valences, an index to the vertex being refined,
    // and the sharpness value.
    std::vector<VertexData> vertex_data;

    const auto vertex_map{GenerateVertexConnectivity(edge_indices, edge_data)};

    int vertex_offset = 0;
    for (const auto& v : vertex_map) {
        for (const auto& adj : v.second.adj_vertices) {
            vertex_indices.emplace_back(adj);
        }

        bool boundary = false;
        for (const auto& adj : v.second.adj_faces) {
            if (adj != -1) {
                vertex_indices.emplace_back(adj);
            } else {
                boundary = true;
            }
        }

        int valence = v.second.adj_vertices.size();
        int face_valence = v.second.adj_faces.size();
        if (boundary) {
            --face_valence;
        }

        vertex_data.emplace_back(vertex_offset, valence, face_valence, v.first, 0.0f);
        vertex_offset += valence + face_valence;

        // Control vertex refinement. TODO: boundary and corner.
        //glm::vec3 new_vertex(0.0f);
        //for (const auto& adj : v.second.adj_vertices) {
        //    new_vertex += vertex_buffer[adj];
        //}

        //for (const auto& adj : v.second.adj_faces) {
        //    new_vertex += vertex_buffer[face_data[adj].inserted_vertex];
        //}

        //float valence_f = valence;
        //new_vertex /= valence_f * valence_f;
        //new_vertex += vertex_buffer[v.first] * (valence_f - 2) / valence_f;
    }

    return std::make_tuple(vertex_indices, vertex_data);
}

EdgeMap GenerateEdgeConnectivity(const std::vector<int>& face_indices, const std::vector<FaceData>& face_data) {
    // Iterate over all faces to obtain the edge connectivity information.
    // As most edges will be discovered twice, we keep track of generated edges in a map. We use a map instead
    // of a set, because we need to update the second face after the edge has already been inserted.
    EdgeMap edges;

    // Iterate over all faces and find their edges.
    for (int i = 0; i < face_data.size(); ++i) {
        int face_offset = face_data[i].offset;
        int face_valence = face_data[i].valence;

        // Loop over each edge of the face.
        for (int v = 0; v < face_valence; ++v) {
            // The last edge is (valence - 1, 0), otherwise it's (v, v + 1).
            int first_index = face_offset + v;
            int second_index = face_offset + ((v == face_valence - 1) ? 0 : v + 1);

            Edge edge{face_indices[first_index], face_indices[second_index]};

            // Check if we've already processed this edge.
            auto found_edge = edges.find(edge);
            if (found_edge == edges.end()) {
                // New edge. Insert the offset into face_data.
                edges.emplace(edge, std::array<int, 2>{{i, -1}});
            } else if (found_edge->second[1] != -1) {
                // Edge has already been found once. Insert the offset into face_data.
                found_edge->second[1] = i;
            } else {
                // Edge found a third time - we do not handle meshes with edges adjacent to more than 2 faces.
                throw std::runtime_error("Edge with valence > 2 found in mesh.");
            }
        }
    }

    return edges;
}

VertexMap GenerateVertexConnectivity(const std::vector<int>& edge_indices, const std::vector<EdgeData>& edge_data) {
    // Iterate over all edges to obtain the vertex connectivity information.
    VertexMap vertices;

    for (const auto& edge : edge_data) {
        int edge_vertex1 = edge_indices[edge.offset];
        int edge_vertex2 = edge_indices[edge.offset + 1];
        int face_offset1 = edge_indices[edge.offset + 2];
        int face_offset2 = edge_indices[edge.offset + 3];

        // Attempt to emplace the vertices into the map, the existing element will be returned if we've found
        // this vertex already.
        auto map_insert1 = vertices.emplace(edge_vertex1, Vertex());
        auto map_insert2 = vertices.emplace(edge_vertex2, Vertex());

        Vertex& vertex1 = map_insert1.first->second;
        Vertex& vertex2 = map_insert2.first->second;

        // Insert the neighbouring vertices of each vertex from this edge.
        vertex1.adj_vertices.insert(edge_vertex2);
        vertex1.adj_faces.insert(face_offset1);
        vertex1.adj_faces.insert(face_offset2);

        vertex2.adj_vertices.insert(edge_vertex1);
        vertex2.adj_faces.insert(face_offset1);
        vertex2.adj_faces.insert(face_offset2);
    }

    return vertices;
}

} // End namespace Renderer
