#include <iostream>

#include "renderer/Render.h"

namespace Renderer {

void PrintUniformBlockOffsets(const GLuint shader_id, const std::string& block_name) {
    GLuint ub_index = glGetUniformBlockIndex(shader_id, block_name.c_str());
    GLint num_uniforms;
    glGetActiveUniformBlockiv(shader_id, ub_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &num_uniforms);
    GLuint indices[num_uniforms];
    glGetActiveUniformBlockiv(shader_id, ub_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                              reinterpret_cast<GLint*>(indices));
    GLint offsets[num_uniforms];
    glGetActiveUniformsiv(shader_id, num_uniforms, indices, GL_UNIFORM_OFFSET, offsets);

    GLint max_name_len;
    glGetProgramiv(shader_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_len);

    std::vector<GLchar> uniform_name(max_name_len, ' ');
    GLsizei name_length;
    for (int i = 0; i < num_uniforms; ++i) {
        glGetActiveUniformName(shader_id, indices[i], max_name_len, &name_length, uniform_name.data());
        for (const auto& c : uniform_name) {
            std::cout << c;
        }
        std::cout << " " << offsets[i] << "\n";

        std::fill(uniform_name.begin(), uniform_name.end(), ' ');
    }

    if (glGetError() != GL_NO_ERROR) {
        std::cout << "Error in PrintUniformBlockOffsets()\n";
    }
}

} // End namespace Renderer.
