#pragma once

#include <string>

#include <GL/glew.h>

namespace Renderer {

class Shader {
public:
    GLuint id;

    Shader(const std::string& vs_path, const std::string& gs_path, const std::string& fs_path);
    Shader(const std::string& vs_path, const std::string& fs_path);
private:
    GLuint CompileShaders(const std::string& vs_str, const std::string& gs_str, const std::string& fs_str);
    GLuint CompileShaders(const std::string& vs_str, const std::string& fs_str);
};

} // End namespace Renderer.
