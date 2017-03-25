#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include "renderer/Camera.h"

namespace Renderer {

Camera::Camera(const glm::vec3& init_pos, const glm::vec3& up_vec, const glm::vec3& look)
        : pos(init_pos)
        , up(glm::normalize(up_vec))
        , look_dir(glm::normalize(look))
        , right_dir(glm::normalize(glm::cross(look_dir, up))) {

    pitch = glm::degrees(glm::asin(look_dir.y));
    yaw = glm::degrees(glm::asin(look_dir.z / glm::cos(glm::radians(pitch))));
}

void Camera::UpdateLookDir(float x_offset, float y_offset) {
    yaw += x_offset;
    pitch += y_offset;
    pitch = std::max(std::min(pitch, 89.0f), -89.0f);

    look_dir.x = glm::cos(glm::radians(pitch)) * glm::cos(glm::radians(yaw));
    look_dir.y = glm::sin(glm::radians(pitch));
    look_dir.z = glm::cos(glm::radians(pitch)) * glm::sin(glm::radians(yaw));
    look_dir = glm::normalize(look_dir);

    right_dir = glm::normalize(glm::cross(look_dir, up));
}

void Camera::Move(Direction dir, float delta_time) {
    const float speed = 5.0f * delta_time;

    switch (dir) {
    case Direction::Up:
        pos += speed * glm::normalize(glm::cross(right_dir, look_dir));
        break;
    case Direction::Down:
        pos -= speed * glm::normalize(glm::cross(right_dir, look_dir));
        break;
    case Direction::Left:
        pos -= speed * right_dir;
        break;
    case Direction::Right:
        pos += speed * right_dir;
        break;
    default:
        break;
    }
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(pos, pos + look_dir, up);
}

} // End namespace Renderer.
