#include <stdexcept>
#include <iostream>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "renderer/Init.h"

namespace Renderer {

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void APIENTRY debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                             GLsizei length, const GLchar* message, const void* user_params) {
    // Ignore unimportant/spammed notifications.
    if (id == 131185) {
        return;
    }

    std::ostringstream debug_message;
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        debug_message << "Source: API, ";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        debug_message << "Source: Window System, ";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        debug_message << "Source: Shader Compiler, ";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        debug_message << "Source: Third Party, ";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        debug_message << "Source: Application, ";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        debug_message << "Source: Other, ";
        break;
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        debug_message << "Type: Error, ";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        debug_message << "Type: Deprecated Behaviour, ";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        debug_message << "Type: Undefined Behaviour, ";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        debug_message << "Type: Portability, ";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        debug_message << "Type: Performance, ";
        break;
    case GL_DEBUG_TYPE_MARKER:
        debug_message << "Type: Marker, ";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        debug_message << "Type: Push Group, ";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        debug_message << "Type: Pop Group, ";
        break;
    case GL_DEBUG_TYPE_OTHER:
        debug_message << "Type: Other, ";
        break;
    }

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        debug_message << "Severity: HIGH, ";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        debug_message << "Severity: medium, ";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        debug_message << "Severity: low, ";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        debug_message << "Severity: notification, ";
        break;
    }

    debug_message << "ID: " << id << "\n";
    debug_message << message;

    std::cout << debug_message.str() << "\n" << std::endl;
}

GLFWwindow* InitGL(int window_width, int window_height) {
    // Set up GLFW.
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    // Remember to disable the debug context for release builds.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Subdivision", nullptr, nullptr);
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

    // Only set the callback if we initialized a debug context correctly.
    GLint context_flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glDebugMessageCallback(debug_callback, nullptr);
    }

    // Get primary monitor resolution.
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - window_width) / 2, (mode->height - window_height) / 2);

    // Register callbacks.
    glfwSetKeyCallback(window, key_callback);

    return window;
}

} // End namespace Renderer.
