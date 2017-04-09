#pragma once

#include <string>
#include <vector>
#include <tuple>

#include <GL/glew.h>

namespace Shader {

using Paths = std::vector<std::tuple<const std::string, GLenum>>;

GLuint Init(const Paths& shader_paths);
GLuint CompileShaders(const Paths& shader_strings);
GLuint CreateShaderObject(const char* shader_source, const GLenum shader_type);
std::string ShaderNameFromEnum(const GLenum shader) noexcept;

} // End namespace Shader.

