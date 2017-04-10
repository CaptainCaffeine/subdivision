#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "externals/tiny_obj_loader.h"
#include "renderer/Connectivity.h"

namespace Renderer {

struct TinyObjMesh;
struct IndexedMesh;

IndexedMesh SubdivideMesh(const TinyObjMesh& obj_data);
void SubdivideFaces(std::vector<FaceDataPtr>& face_data, std::vector<glm::vec3>& vertex_buffer, int tess_level);

void InsertFaceVertex(const FaceData& face, std::vector<glm::vec3>& vertex_buffer);
void InsertEdgeVertex(const EdgeData& edge, std::vector<glm::vec3>& vertex_buffer);
void RefineControlVertex(const VertexData& vertex, std::vector<glm::vec3>& vertex_buffer);
void CreateNewFaces(std::vector<glm::vec3>& vertex_buffer,
                    std::vector<FaceDataPtr>& face_data,
                    std::vector<EdgeData>& edge_data,
                    std::vector<VertexData>& vertex_data);

} // End namespace Renderer
