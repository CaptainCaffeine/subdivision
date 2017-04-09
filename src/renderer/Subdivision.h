#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "externals/tiny_obj_loader.h"
#include "renderer/Connectivity.h"

namespace Renderer {

struct TinyObjMesh;
struct IndexedMesh;

IndexedMesh SubdivideMesh(const TinyObjMesh& obj_data);
void SubdivideFaces(std::vector<FaceData>& face_data, std::vector<glm::vec3>& vertex_buffer, bool first_step);

// The data vectors are not const because the function needs to set the inserted vertex index.
void InsertVertices(std::vector<glm::vec3>& vertex_buffer, std::vector<FaceData>& face_data,
                    std::vector<EdgeData>& edge_data, std::vector<VertexData>& vertex_data);
void CreateNewFaces(const std::vector<glm::vec3>& vertex_buffer,
                    std::vector<FaceData>& face_data,
                    const std::vector<VertexData>& vertex_data);

} // End namespace Renderer
