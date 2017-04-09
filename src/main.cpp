#include <iostream>

#include "renderer/Init.h"
#include "renderer/Shader.h"
#include "renderer/Render.h"

int main() {
    constexpr int window_width = 1024, window_height = 1024;
    GLFWwindow* window;

    Shader::Paths quad{
        {"shaders/passthrough_vertex_shader.glsl", GL_VERTEX_SHADER},
        {"shaders/tess_control_quad.glsl", GL_TESS_CONTROL_SHADER},
        {"shaders/tess_eval_quad.glsl", GL_TESS_EVALUATION_SHADER},
        {"shaders/fragment_shader.glsl", GL_FRAGMENT_SHADER}
    };

    Shader::Paths subd{
        {"shaders/passthrough_vertex_shader.glsl", GL_VERTEX_SHADER},
        {"shaders/tess_control_bspline.glsl", GL_TESS_CONTROL_SHADER},
        {"shaders/tess_eval_bspline.glsl", GL_TESS_EVALUATION_SHADER},
        {"shaders/fragment_shader.glsl", GL_FRAGMENT_SHADER}
    };

    Shader::Paths light_quad{
        {"shaders/passthrough_vertex_shader.glsl", GL_VERTEX_SHADER},
        {"shaders/tess_control_quad.glsl", GL_TESS_CONTROL_SHADER},
        {"shaders/tess_eval_quad.glsl", GL_TESS_EVALUATION_SHADER},
        {"shaders/light_fragment_shader.glsl", GL_FRAGMENT_SHADER}
    };

    try {
        window = Renderer::InitGL(window_width, window_height);
        std::vector<GLuint> shaders{Shader::Init(quad), Shader::Init(subd), Shader::Init(light_quad)};
        Renderer::RenderLoop(window, shaders, window_width, window_height);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        glfwTerminate();
        return 1;
    }

    glfwTerminate();
    return 0;
}
