#pragma once

#include <glm/glm.hpp>

namespace Renderer {

enum class Direction {Up, Down, Left, Right};

class Camera {
public:
    Camera(const glm::vec3& init_pos, const glm::vec3& up_vec, const glm::vec3& look);
    void UpdateLookDir(float x_offset, float y_offset);
    void Move(Direction dir, float delta_time);
    glm::mat4 GetViewMatrix() const;
private:
    glm::vec3 pos, up, look_dir, right_dir;
    float yaw, pitch;
};

} // End namespace Renderer.
