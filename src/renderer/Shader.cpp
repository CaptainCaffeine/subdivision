#include <stdexcept>
#include <fstream>
#include <sstream>

#include "renderer/Shader.h"

namespace Shader {

GLuint Init(const std::vector<std::tuple<std::string, GLenum>>& shader_paths) {
    std::vector<std::tuple<std::string, GLenum>> shader_contents;
    for (const auto& shader_path : shader_paths) {
        std::ifstream shader_source{std::get<0>(shader_path)};
        if (!shader_source) {
            throw std::runtime_error("Error when attempting to open " + std::get<0>(shader_path));
        }

        std::ostringstream shader_stream;
        shader_stream << shader_source.rdbuf();

        shader_contents.emplace_back(shader_stream.str(), std::get<1>(shader_path));
    }

    return CompileShaders(shader_contents);
}

GLuint CompileShaders(const std::vector<std::tuple<std::string, GLenum>>& shader_strings) {
    std::vector<GLuint> shader_objects;
    for (const auto& s : shader_strings) {
        shader_objects.push_back(CreateShaderObject(std::get<0>(s).c_str(), std::get<1>(s)));
    }

    // Generate the shader program object & link the shaders.
    GLuint shader_program;
    shader_program = glCreateProgram();
    for (const auto& shader : shader_objects) {
        glAttachShader(shader_program, shader);
    }
    glLinkProgram(shader_program);

    // Check if the program linked successfully.
    GLint success;
    GLchar info_log[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
        throw std::runtime_error("Shader program linking failed:\n" + std::string(info_log));
    }

    for (auto& shader : shader_objects) {
        glDeleteShader(shader);
    }

    return shader_program;
}

GLuint CreateShaderObject(const char* shader_source, const GLenum shader_type) {
    // Generate the shader object & compile the shader source.
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &shader_source, nullptr);
    glCompileShader(shader);

    // Check if the vertex shader compiled successfully.
    GLint success;
    GLchar info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::string shader_name = ShaderNameFromEnum(shader_type);
        throw std::runtime_error(shader_name + " shader compilation failed:\n" + std::string(info_log));
    }

    return shader;
}

std::string ShaderNameFromEnum(const GLenum shader) noexcept {
    switch(shader) {
    case GL_VERTEX_SHADER:
        return "Vertex";
    case GL_FRAGMENT_SHADER:
        return "Fragment";
    case GL_TESS_CONTROL_SHADER:
        return "Tessellation Control";
    case GL_TESS_EVALUATION_SHADER:
        return "Tessellation Evaluation";
    case GL_GEOMETRY_SHADER:
        return "Geometry";
    case GL_COMPUTE_SHADER:
        return "Compute";
    default:
        return "Unknown shader or invalid enum";
    }
}

} // End namespace Shader.
