#include <GLFW/glfw3.h>

#include "renderer/Input.h"
#include "renderer/Camera.h"

namespace Renderer {

Input::Input() : prev_state(GLFW_RELEASE) {}

void Input::HandleInput(GLFWwindow* window, Camera& camera) {
    UpdateDeltaTime();

    glfwPollEvents();

    HandleCursor(window, camera);
    HandleWASD(window, camera);
}

void Input::UpdateDeltaTime() {
    float current_frame_time = glfwGetTime();
    delta_time = current_frame_time - last_frame_time;
    last_frame_time = current_frame_time;
}

void Input::HandleCursor(GLFWwindow* window, Camera& camera) {
    mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouse_state == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (mouse_state != prev_state) {
            // The mouse button was just pressed, so the next movements will be made relative to this point.
            prev_xpos = xpos;
            prev_ypos = ypos;
        } else {
            GLfloat sensitivity{4.0f * delta_time};
            GLfloat x_offset = (xpos - prev_xpos) * sensitivity;
            GLfloat y_offset = (prev_ypos - ypos) * sensitivity;
            prev_xpos = xpos;
            prev_ypos = ypos;

            camera.UpdateLookDir(x_offset, y_offset);
        }
    }
    prev_state = mouse_state;
}

void Input::HandleWASD(GLFWwindow* window, Camera& camera) const {
    bool w_pressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool a_pressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool s_pressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool d_pressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (w_pressed && !s_pressed) {
        camera.Move(Direction::Up, delta_time);
    } else if (s_pressed && !w_pressed) {
        camera.Move(Direction::Down, delta_time);
    }

    if (a_pressed && !d_pressed) {
        camera.Move(Direction::Left, delta_time);
    } else if (d_pressed && !a_pressed) {
        camera.Move(Direction::Right, delta_time);
    }
}

} // End namespace Renderer.
