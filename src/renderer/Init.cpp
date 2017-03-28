#include <stdexcept>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "renderer/Init.h"

namespace Renderer {

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

GLFWwindow* InitGL(int window_width, int window_height) {
    // Set up GLFW.
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // In case this is compiled on macOS.
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4); // Enable 4x MSAA.

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Modeling OpenGL Renderer", nullptr, nullptr);
    if (window == nullptr) {
        throw std::runtime_error("Failed to create a GLFW window.");
    }
    glfwMakeContextCurrent(window);

    // Set up GLEW.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW.");
    }

    // Set up OpenGL viewport.
    int win_width, win_height;
    glfwGetFramebufferSize(window, &win_width, &win_height);
    glViewport(0, 0, win_width, win_height);

    glEnable(GL_DEPTH_TEST);

    // Get primary monitor resolution.
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - window_width) / 2, (mode->height - window_height) / 2);

    // Register callbacks.
    glfwSetKeyCallback(window, key_callback);

    return window;
}

} // End namespace Renderer.
