#pragma once

#include <string>
#include <vector>
#include <tuple>

#include <GL/glew.h>

namespace Shader {

GLuint Init(const std::vector<std::tuple<std::string, GLenum>>& shader_paths);
GLuint CompileShaders(const std::vector<std::tuple<std::string, GLenum>>& shader_strings);
GLuint CreateShaderObject(const char* shader_source, const GLenum shader_type);
std::string ShaderNameFromEnum(const GLenum shader) noexcept;

} // End namespace Shader.

