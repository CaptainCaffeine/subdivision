#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "renderer/Render.h"
#include "renderer/Shader.h"
#include "renderer/Input.h"
#include "renderer/Camera.h"
#include "renderer/Mesh.h"

namespace Renderer {

void RenderLoop(GLFWwindow* window, const std::vector<Shader>& shaders, float win_width, float win_height) {
    static_assert(sizeof(glm::vec3) == sizeof(GLfloat) * 3, "glm::vec3 is not 3 packed floats on this platform.");

    Material cube_mat{{0.0f, 0.7f, 0.54f}, {0.0f, 0.7f, 0.54f}, {0.5f, 0.5f, 0.5f}, 64.0f};
    Mesh cube{CubeVertices(), cube_mat};
    Mesh quad_cube{CubeQuads(), cube_mat};

    Input input;
    Camera camera{{0.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};

    DirLight dir_light{{0.0f, -1.0f, 0.0f}, {0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}};
    std::vector<PointLight> point_lights{
        {{1.4f, 2.8f, 0.6f}, {0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, 1.0f, 0.09f, 0.032f},
        {{-2.0f, 2.2f, -1.8f}, {0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, 1.0f, 0.09f, 0.032f}
    };

    const Shader &regular_shader{shaders[0]}, &light_shader{shaders[1]}, &tess_shader{shaders[2]};

    const bool dir_light_enabled = false, point_light_enabled = true;

    GLuint matrices_UBO = SetMatricesUBO(shaders, win_width/win_height);
    GLuint lights_UBO = SetLightsUBO(shaders, dir_light_enabled, point_light_enabled, dir_light, point_lights);

//    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    while (!glfwWindowShouldClose(window)) {
        input.HandleInput(window, camera);

        glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(regular_shader.id);

        // Camera.
        glm::mat4 view = camera.GetViewMatrix();
        glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Lights.
        glBindBuffer(GL_UNIFORM_BUFFER, lights_UBO);

        if (dir_light_enabled) {
            glm::vec3 view_space_dir = glm::vec3(view * glm::vec4(dir_light.direction, 0.0f));
            glBufferSubData(GL_UNIFORM_BUFFER, 16, 16, glm::value_ptr(view_space_dir));
        }

        if (point_light_enabled) {
            for (std::size_t i = 0; i < point_lights.size(); ++i) {
                unsigned int offset = 80 * i;
                glm::vec3 view_space_pos = glm::vec3(view * glm::vec4(point_lights[i].position, 1.0f));
                glBufferSubData(GL_UNIFORM_BUFFER, offset + 80, 16, glm::value_ptr(view_space_pos));
            }
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        
        // Models.
        cube.model = glm::mat4(1.0f);
        cube.model = glm::rotate(cube.model,
                                 static_cast<float>(glm::radians(glfwGetTime() * 5.0f)),
                                 {1.0f, 0.0f, 0.0f});
        cube.model = glm::rotate(cube.model,
                                 static_cast<float>(glm::radians(glfwGetTime() * 4.0f)),
                                 {0.0f, 1.0f, 0.0f});
        DrawMesh(regular_shader.id, cube, view);

        if (point_light_enabled) {
            // Light cube.
            glUseProgram(light_shader.id);

            for (std::size_t i = 0; i < point_lights.size(); ++i) {
                cube.model = glm::mat4(1.0f);
                cube.model = glm::translate(cube.model, point_lights[i].position);
                cube.model = glm::scale(cube.model, glm::vec3(0.4f));
                DrawMesh(light_shader.id, cube, view);
            }
        }

        // Explode cube.
        glUseProgram(tess_shader.id);
//        glUniform1f(glGetUniformLocation(tess_shader.id, "time"), glfwGetTime());
        
        quad_cube.model = glm::mat4(1.0f);
        quad_cube.model = glm::translate(quad_cube.model, {0.0f, 2.0f, -2.5f});
        quad_cube.model = glm::rotate(quad_cube.model, glm::radians(22.0f), {1.0f, 0.0f, 0.0f});
        quad_cube.model = glm::rotate(quad_cube.model,
                                 static_cast<float>(glm::radians(glfwGetTime() * 4.0f)),
                                 {0.0f, 1.0f, 0.0f});
        DrawMesh(tess_shader.id, quad_cube, view, GL_PATCHES);

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }
}

void DrawMesh(const GLuint shader_id, const Mesh& mesh, const glm::mat4& view_matrix, const GLenum mesh_type) {
    SetMaterial(shader_id, mesh.mat);

    glBindVertexArray(mesh.VAO);

    glUniformMatrix4fv(glGetUniformLocation(shader_id, "model"), 1, GL_FALSE, glm::value_ptr(mesh.model));

    GLint normal_mat_loc = glGetUniformLocation(shader_id, "normal_mat");
    glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(view_matrix * mesh.model)));
    glUniformMatrix3fv(normal_mat_loc, 1, GL_FALSE, glm::value_ptr(normal_matrix));

    glDrawArrays(mesh_type, 0, mesh.vertices.size());
}

GLuint CreateUBO(std::size_t buffer_size) {
    GLuint UBO;
    glGenBuffers(1, &UBO);

    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferData(GL_UNIFORM_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return UBO;
}

GLuint SetMatricesUBO(const std::vector<Shader>& shaders, float aspect) {
    constexpr int bind_index = 0;
    for (const auto& shader : shaders) {
        glUniformBlockBinding(shader.id, glGetUniformBlockIndex(shader.id, "Matrices"), bind_index);
    }

    std::size_t ubo_size = 2 * sizeof(glm::mat4);
    GLuint matrices_UBO = CreateUBO(ubo_size);
    glBindBufferRange(GL_UNIFORM_BUFFER, bind_index, matrices_UBO, 0, ubo_size);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(proj));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return matrices_UBO;
}

GLuint SetLightsUBO(const std::vector<Shader>& shaders, bool dir_enable, bool point_enable,
                    const DirLight& dir_light, const std::vector<PointLight>& point_lights) {
    constexpr int bind_index = 1;

    // No Lights uniform block in the light fragment shader.
    glUniformBlockBinding(shaders[0].id, glGetUniformBlockIndex(shaders[0].id, "Lights"), bind_index);
    glUniformBlockBinding(shaders[2].id, glGetUniformBlockIndex(shaders[2].id, "Lights"), bind_index);

    std::size_t ubo_size = 80 + point_lights.size() * 80;
    GLuint lights_UBO = CreateUBO(ubo_size);
    glBindBufferRange(GL_UNIFORM_BUFFER, bind_index, lights_UBO, 0, ubo_size);

    glBindBuffer(GL_UNIFORM_BUFFER, lights_UBO);

    unsigned int dir_enable_4bytes = static_cast<unsigned int>(dir_enable);
    unsigned int point_enable_4bytes = static_cast<unsigned int>(point_enable);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 4, &dir_enable_4bytes);
    glBufferSubData(GL_UNIFORM_BUFFER, 4, 4, &point_enable_4bytes);

    glBufferSubData(GL_UNIFORM_BUFFER, 16, 16, glm::value_ptr(dir_light.direction));
    glBufferSubData(GL_UNIFORM_BUFFER, 32, 16, glm::value_ptr(dir_light.ambient));
    glBufferSubData(GL_UNIFORM_BUFFER, 48, 16, glm::value_ptr(dir_light.diffuse));
    glBufferSubData(GL_UNIFORM_BUFFER, 64, 16, glm::value_ptr(dir_light.specular));

    for (std::size_t i = 0; i < point_lights.size(); ++i) {
        unsigned int offset = 80 * i;

        glBufferSubData(GL_UNIFORM_BUFFER, offset + 80, 16, glm::value_ptr(point_lights[i].position));
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 96, 16, glm::value_ptr(point_lights[i].ambient));
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 112, 16, glm::value_ptr(point_lights[i].diffuse));
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 128, 16, glm::value_ptr(point_lights[i].specular));
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 140, 4, &point_lights[i].constant);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 144, 4, &point_lights[i].linear);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 148, 4, &point_lights[i].quadratic);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return lights_UBO;
}

void SetMaterial(const GLuint shader_id, const Material& mat) {
    GLint ambient_location = glGetUniformLocation(shader_id, "material.ambient");
    GLint diffuse_location = glGetUniformLocation(shader_id, "material.diffuse");
    GLint specular_location = glGetUniformLocation(shader_id, "material.specular");
    GLint shininess_location = glGetUniformLocation(shader_id, "material.shininess");

    glUniform3f(ambient_location, mat.ambient.r, mat.ambient.g, mat.ambient.b);
    glUniform3f(diffuse_location, mat.diffuse.r, mat.diffuse.g, mat.diffuse.b);
    glUniform3f(specular_location, mat.specular.r, mat.specular.g, mat.specular.b);
    glUniform1f(shininess_location, mat.shininess);
}

std::vector<glm::vec3> CubeVertices() {
    return {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},
            { 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},
            { 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},
            { 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},
            {-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},
            {-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},

            {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},
            { 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},
            { 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},
            { 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},
            {-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},
            {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},

            {-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f},
            {-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f},
            {-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f},
            {-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f},
            {-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f},
            {-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f},

            { 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f},
            { 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f},
            { 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f},
            { 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f},
            { 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f},
            { 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f},

            {-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f},
            { 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f},
            { 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f},
            { 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f},
            {-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f},
            {-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f},

            {-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f},
            { 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f},
            { 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f},
            { 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f},
            {-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f},
            {-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}};
}

std::vector<glm::vec3> CubeQuads() {
    return {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},
            {-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},
            { 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},
            { 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f},

            {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},
            { 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},
            { 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},
            {-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f},

            {-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f},
            {-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f},
            {-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f},
            {-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f},

            { 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f},
            { 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f},
            { 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f},
            { 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f},

            {-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f},
            { 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f},
            { 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f},
            {-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f},

            {-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f},
            {-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f},
            { 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f},
            { 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}};
}

} // End namespace Renderer.
