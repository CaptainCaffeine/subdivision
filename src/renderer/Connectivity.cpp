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

void GenerateMeshConnectivity(const std::vector<glm::vec3>& vertex_buffer,
                              const std::vector<tinyobj::mesh_t>& meshes) {
    int inserted_vertex = vertex_buffer.size();

    // Contains indices into the vertex_buffer.
    std::vector<int> face_indices;
    // Contains offsets into face_indices, valences, and the index of the inserted vertex for each face.
    std::vector<int> face_data;
    std::tie(face_indices, face_data) = PopulateFaceBuffers(meshes, inserted_vertex);
    inserted_vertex += face_data.size() / 3;

    // Contains indices for the two adjacent control vertices, and the adjacent inserted face vertices.
    std::vector<int> edge_indices;
    // Contains offsets into edge_indices, the index of the inserted edge index, and the sharpness value.
    std::vector<int> edge_data;
    std::tie(edge_indices, edge_data) = PopulateEdgeBuffers(face_indices, face_data, inserted_vertex);
    inserted_vertex += edge_data.size() / 3;

    // Contains indices to the adjacent control mesh vertices and inserted face vertices.
    std::vector<int> vertex_indices;
    // Contains offsets into vertex_indices, valences, an index to the vertex being refined, and the sharpness value.
    std::vector<int> vertex_data;

    std::tie(vertex_indices, vertex_data) = PopulateVertexBuffers(edge_indices, edge_data, inserted_vertex);

    // Now what? Need to make new faces...
}

BufferPair PopulateFaceBuffers(const std::vector<tinyobj::mesh_t>& meshes, int inserted_vertex) {
    // Contains indices into the vertex_buffer.
    std::vector<int> face_indices;
    // Contains offsets into face_indices, valences, and the index of the inserted vertex for each face.
    std::vector<int> face_data;

    // Get the face-vertex data from the provided .obj.
    // Usually only one shape in an .obj file, but iterate over them just in case.
    for (const auto& mesh : meshes) {
        // Face indices.
        std::transform(mesh.indices.cbegin(), mesh.indices.cend(), std::back_inserter(face_indices),
                       [](const tinyobj::index_t& index) { return index.vertex_index; });

        // Face data.
        int face_offset = 0;
        for (const auto& valence : mesh.num_face_vertices) {
            face_data.emplace_back(face_offset);
            face_data.emplace_back(valence);
            face_data.emplace_back(inserted_vertex++);
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

BufferPair PopulateEdgeBuffers(const std::vector<int>& face_indices, const std::vector<int>& face_data,
                               int inserted_vertex) {
    // Contains indices for the two adjacent control vertices, and the adjacent inserted face vertices.
    std::vector<int> edge_indices;
    // Contains offsets into edge_indices, the index of the inserted edge index, and the sharpness value.
    std::vector<int> edge_data;

    auto edge_map{GenerateEdgeConnectivity(face_indices, face_data)};

    // Keep value of inserted_vertex from above.
    int edge_offset = 0;
    for (const auto& e : edge_map) {
        edge_indices.emplace_back(e.first.vertex1);
        edge_indices.emplace_back(e.first.vertex2);
        // FIXME: Should these be references to the faces (offsets into face_data) instead?
        edge_indices.emplace_back(e.second[0]);
        edge_indices.emplace_back(e.second[1]);

        //bool boundary = e.second[1] == -1;

        edge_data.emplace_back(edge_offset);
        edge_data.emplace_back(inserted_vertex++); // Might not need this
        edge_data.emplace_back(0); // TODO: sharpness
        edge_offset += 4;

        // Edge vertex insertion. TODO: boundary edges
        //glm::vec3 new_vertex{vertex_buffer[e.vertex1] +
        //                     vertex_buffer[e.vertex2] +
        //                     vertex_buffer[e.face1] +
        //                     vertex_buffer[e.face2]};
        //vertex_buffer.push_back(new_vertex / 4.0f);
    }

    return std::make_tuple(edge_indices, edge_data);
}

BufferPair PopulateVertexBuffers(const std::vector<int>& edge_indices, const std::vector<int>& edge_data,
                                 int inserted_vertex) {
    // Contains indices to the adjacent control mesh vertices and inserted face vertices.
    std::vector<int> vertex_indices;
    // Contains offsets into vertex_indices, valences, an index to the vertex being refined, and the sharpness value.
    std::vector<int> vertex_data;

    auto vertex_map{GenerateVertexConnectivity(edge_indices, edge_data)};

    int vertex_offset = 0;
    for (const auto& v : vertex_map) {
        bool boundary = false;
        for (const auto& neighbour : v.second) {
            if (neighbour != -1) {
                vertex_indices.emplace_back(neighbour);
            } else {
                boundary = true;
            }
        }

        int valence = v.second.size();

        vertex_data.emplace_back(vertex_offset);
        vertex_data.emplace_back(valence);
        vertex_data.emplace_back(v.first);
        vertex_data.emplace_back(0); // TODO: sharpness
        // Inserted vertex?

        vertex_offset += valence;

        // Control vertex refinement. TODO: boundary and corner.
        //glm::vec3 new_vertex(0.0f);
        //for (const auto& neighbour : v.second) {
        //    new_vertex += vertex_buffer[neighbour];
        //}

        //float valence_f = valence;
        //new_vertex /= valence_f * valence_f;
        //new_vertex += vertex_buffer[v.first] * (valence_f - 2) / valence_f;
    }

    return std::make_tuple(vertex_indices, vertex_data);
}

EdgeMap GenerateEdgeConnectivity(const std::vector<int>& face_indices, const std::vector<int>& face_data) {
    // Iterate over all faces to obtain the edge connectivity information.
    // As most edges will be discovered twice, we keep track of generated edges in a map. We use a map instead
    // of a set, because we need to update the second face after the edge has already been inserted.
    EdgeMap edges;

    // Iterate over all faces and find their edges.
    for (std::size_t i = 0; i < face_data.size(); i += 3) {
        int face_offset = face_data[i];
        int face_valence = face_data[i + 1];
        int face_vertex = face_data[i + 2];

        // Loop over each edge of the face.
        for (int v = 0; v < face_valence - 1; ++v) {
            Edge edge{face_indices[face_offset + v], face_indices[face_offset + v + 1]};

            // Check if we've already processed this edge.
            auto found_edge = edges.find(edge);
            if (found_edge == edges.end()) {
                // New edge.
                edges.emplace(edge, std::array<int, 2>{{face_vertex, -1}});
            } else if (found_edge->second[1] != -1) {
                // Edge has already been found once.
                found_edge->second[1] = face_vertex;
            } else {
                // Edge found a third time - we do not handle meshes with edges adjacent to more than 2 faces.
                throw std::runtime_error("Edge with valence > 2 found in mesh.");
            }
        }
    }

    return edges;
}

VertexMap GenerateVertexConnectivity(const std::vector<int>& edge_indices, const std::vector<int>& edge_data) {
    // Iterate over all edges to obtain the vertex connectivity information.
    VertexMap vertices;

    for (std::size_t i = 0; i < edge_data.size(); i += 3) {
        int edge_offset = edge_data[i];
        //int edge_vertex = edge_data[i + 1];
        //int edge_sharpness = edge_data[i + 2];

        int edge_v1 = edge_indices[edge_offset];
        int edge_v2 = edge_indices[edge_offset + 1];
        int face_v1 = edge_indices[edge_offset + 2];
        int face_v2 = edge_indices[edge_offset + 3];

        // Attempt to emplace the vertices into the map, the existing element will be returned if we've found
        // this vertex already.
        auto map_insert1 = vertices.emplace(edge_v1, std::unordered_set<int>());
        auto map_insert2 = vertices.emplace(edge_v2, std::unordered_set<int>());

        std::unordered_set<int>& neighbours1 = map_insert1.first->second;
        std::unordered_set<int>& neighbours2 = map_insert2.first->second;

        // Insert the neighbouring vertices of each vertex from this edge.
        neighbours1.insert(edge_v2);
        neighbours1.insert(face_v1);
        neighbours1.insert(face_v2);

        neighbours2.insert(edge_v1);
        neighbours2.insert(face_v1);
        neighbours2.insert(face_v2);
    }

    return vertices;
}

} // End namespace Renderer
