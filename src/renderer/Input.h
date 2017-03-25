#pragma once

struct GLFWwindow;

namespace Renderer {

class Camera;

class Input {
public:
    Input();
    void HandleInput(GLFWwindow* window, Camera& camera);
private:
    float delta_time = 0.0f, last_frame_time = 0.0f;
    int mouse_state, prev_state;
    double prev_xpos = 0.0, prev_ypos = 0.0;

    void UpdateDeltaTime();
    void HandleCursor(GLFWwindow* window, Camera& camera);
    void HandleWASD(GLFWwindow* window, Camera& camera) const;
};

} // End namespace Renderer.
