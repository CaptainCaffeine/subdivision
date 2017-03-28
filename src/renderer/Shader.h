#pragma once

#include <string>

#include <GL/glew.h>

namespace Renderer {

class Shader {
public:
    GLuint id;

    Shader(const std::string& vs_path, const std::string& tcs_path,
           const std::string& tes_path, const std::string& fs_path);
    Shader(const std::string& vs_path, const std::string& gs_path, const std::string& fs_path);
    Shader(const std::string& vs_path, const std::string& fs_path);
private:
    GLuint CompileShaders(const std::string& vs_str, const std::string& tcs_str,
                          const std::string& tes_str, const std::string& fs_str);
    GLuint CompileShaders(const std::string& vs_str, const std::string& gs_str, const std::string& fs_str);
    GLuint CompileShaders(const std::string& vs_str, const std::string& fs_str);

    GLuint CreateShaderObject(const char* shader_source, GLenum shader_type, const std::string& shader_name);
};

} // End namespace Renderer.
