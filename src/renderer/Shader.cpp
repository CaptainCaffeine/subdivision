#include <stdexcept>
#include <fstream>
#include <sstream>

#include "renderer/Shader.h"

namespace Renderer {

Shader::Shader(const std::string& vs_path, const std::string& tcs_path,
               const std::string& tes_path, const std::string& fs_path) {
    std::ifstream vs_source(vs_path), tcs_source(tcs_path), tes_source(tes_path), fs_source(fs_path);
    if (!vs_source) {
        throw std::runtime_error("Error when attempting to open " + vs_path);
    }
    if (!tcs_source) {
        throw std::runtime_error("Error when attempting to open " + tcs_path);
    }
    if (!tes_source) {
        throw std::runtime_error("Error when attempting to open " + tes_path);
    }
    if (!fs_source) {
        throw std::runtime_error("Error when attempting to open " + fs_path);
    }

    std::ostringstream vs_contents, tcs_contents, tes_contents, fs_contents;
    vs_contents << vs_source.rdbuf();
    tcs_contents << tcs_source.rdbuf();
    tes_contents << tes_source.rdbuf();
    fs_contents << fs_source.rdbuf();

    id = CompileShaders(vs_contents.str(), tcs_contents.str(), tes_contents.str(), fs_contents.str());
}

Shader::Shader(const std::string& vs_path, const std::string& gs_path, const std::string& fs_path) {
    std::ifstream vs_source(vs_path), gs_source(gs_path), fs_source(fs_path);
    if (!vs_source) {
        throw std::runtime_error("Error when attempting to open " + vs_path);
    }
    if (!gs_source) {
        throw std::runtime_error("Error when attempting to open " + gs_path);
    }
    if (!fs_source) {
        throw std::runtime_error("Error when attempting to open " + fs_path);
    }

    std::ostringstream vs_contents, gs_contents, fs_contents;
    vs_contents << vs_source.rdbuf();
    gs_contents << gs_source.rdbuf();
    fs_contents << fs_source.rdbuf();

    id = CompileShaders(vs_contents.str(), gs_contents.str(), fs_contents.str());
}

Shader::Shader(const std::string& vs_path, const std::string& fs_path) {
    std::ifstream vs_source(vs_path), fs_source(fs_path);
    if (!vs_source) {
        throw std::runtime_error("Error when attempting to open " + vs_path);
    }
    if (!fs_source) {
        throw std::runtime_error("Error when attempting to open " + fs_path);
    }

    std::ostringstream vs_contents, fs_contents;
    vs_contents << vs_source.rdbuf();
    fs_contents << fs_source.rdbuf();

    id = CompileShaders(vs_contents.str(), fs_contents.str());
}

GLuint Shader::CompileShaders(const std::string& vs_str, const std::string& tcs_str,
                              const std::string& tes_str, const std::string& fs_str) {
    auto vs_c_str = vs_str.c_str(), tcs_c_str = tcs_str.c_str(), tes_c_str = tes_str.c_str(), fs_c_str = fs_str.c_str();

    GLuint vertex_shader = CreateShaderObject(vs_c_str, GL_VERTEX_SHADER, "Vertex");
    GLuint control_shader = CreateShaderObject(tcs_c_str, GL_TESS_CONTROL_SHADER, "Tessellation Control");
    GLuint eval_shader = CreateShaderObject(tes_c_str, GL_TESS_EVALUATION_SHADER, "Tessellation Evaluation");
    GLuint fragment_shader = CreateShaderObject(fs_c_str, GL_FRAGMENT_SHADER, "Fragment");

    // Generate the shader program object & link the shaders.
    GLuint shader_program;
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, control_shader);
    glAttachShader(shader_program, eval_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // Check if the program linked successfully.
    GLint success;
    GLchar info_log[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
        throw std::runtime_error("Shader program linking failed:\n" + std::string(info_log));
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(control_shader);
    glDeleteShader(eval_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

GLuint Shader::CompileShaders(const std::string& vs_str, const std::string& gs_str, const std::string& fs_str) {
    auto vs_c_str = vs_str.c_str(), gs_c_str = gs_str.c_str(), fs_c_str = fs_str.c_str();

    GLuint vertex_shader = CreateShaderObject(vs_c_str, GL_VERTEX_SHADER, "Vertex");
    GLuint geometry_shader = CreateShaderObject(gs_c_str, GL_GEOMETRY_SHADER, "Geometry");
    GLuint fragment_shader = CreateShaderObject(fs_c_str, GL_FRAGMENT_SHADER, "Fragment");

    // Generate the shader program object & link the shaders.
    GLuint shader_program;
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, geometry_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // Check if the program linked successfully.
    GLint success;
    GLchar info_log[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
        throw std::runtime_error("Shader program linking failed:\n" + std::string(info_log));
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(geometry_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

GLuint Shader::CompileShaders(const std::string& vs_str, const std::string& fs_str) {
    auto vs_c_str = vs_str.c_str(), fs_c_str = fs_str.c_str();

    GLuint vertex_shader = CreateShaderObject(vs_c_str, GL_VERTEX_SHADER, "Vertex");
    GLuint fragment_shader = CreateShaderObject(fs_c_str, GL_FRAGMENT_SHADER, "Fragment");

    // Generate the shader program object & link the shaders.
    GLuint shader_program;
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // Check if the program linked successfully.
    GLint success;
    GLchar info_log[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
        throw std::runtime_error("Shader program linking failed:\n" + std::string(info_log));
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

GLuint Shader::CreateShaderObject(const char* shader_source, GLenum shader_type, const std::string& shader_name) {
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
        throw std::runtime_error(shader_name + " shader compilation failed:\n" + std::string(info_log));
    }

    return shader;
}

} // End namespace Renderer.
