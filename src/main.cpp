#include <iostream>

#include "renderer/Init.h"
#include "renderer/Shader.h"
#include "renderer/Render.h"

int main() {
    const int window_width = 1024, window_height = 1024;
    GLFWwindow* window;

    // The try-catch is to catch initialization errors in InitGL and the Shader constructor. RenderLoop is in the
    // try block because it needs the Shader objects, but it shouldn't throw an exception.
    try {
        window = Renderer::InitGL(window_width, window_height);
        std::vector<Renderer::Shader> shaders{
            {"shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl"},
            {"shaders/vertex_shader.glsl", "shaders/light_fragment_shader.glsl"},
            {"shaders/passthrough_vertex_shader.glsl", "shaders/geo_shader.glsl", "shaders/fragment_shader.glsl"}};
        Renderer::RenderLoop(window, shaders, window_width, window_height);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        glfwTerminate();
        return 1;
    }

    glfwTerminate();
    return 0;
}
