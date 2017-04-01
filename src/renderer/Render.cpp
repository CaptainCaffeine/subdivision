#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "renderer/Render.h"
#include "renderer/Shader.h"
#include "renderer/Input.h"
#include "renderer/Camera.h"
#include "renderer/Mesh.h"

namespace Renderer {

void RenderLoop(GLFWwindow* window, const std::vector<GLuint>& shaders, float win_width, float win_height) {
    static_assert(sizeof(glm::vec3) == sizeof(GLfloat) * 3, "glm::vec3 is not 3 packed floats on this platform.");

    Material cube_mat{{0.0f, 0.7f, 0.54f}, {0.0f, 0.7f, 0.54f}, {0.5f, 0.5f, 0.5f}, 64.0f};
    Mesh cube{CubeVertices(), cube_mat};
    Mesh quad_cube{PatchVerts(), cube_mat};
    Mesh big_guy{Mesh::LoadObjectFromFile("../models/bigguy.obj"), cube_mat};
    Mesh monster_frog{Mesh::LoadObjectFromFile("../models/monsterfrog.obj"), cube_mat};

    Input input;
    Camera camera{{0.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};

    DirLight dir_light{{0.0f, -1.0f, 0.0f}, {0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}};
    std::vector<PointLight> point_lights{
        {{1.4f, 2.8f, 0.6f}, {0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, 1.0f, 0.09f, 0.032f},
        {{-2.0f, 2.2f, -1.8f}, {0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, 1.0f, 0.09f, 0.032f}
    };

    const GLuint &regular_shader{shaders[0]}, &light_shader{shaders[1]}, &tess_shader{shaders[2]};

    const bool dir_light_enabled = false, point_light_enabled = true;

    GLuint matrices_UBO = SetMatricesUBO(win_width/win_height);
    GLuint lights_UBO = SetLightsUBO(dir_light_enabled, point_light_enabled, dir_light, point_lights);
    SetTessellationUBO();

//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPatchParameteri(GL_PATCH_VERTICES, 16);

    while (!glfwWindowShouldClose(window)) {
        input.HandleInput(window, camera);

        glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(regular_shader);

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
        cube.model = glm::translate(cube.model, {0.0f, 2.0f, -2.5f});
        cube.model = glm::rotate(cube.model,
                                 static_cast<float>(glm::radians(glfwGetTime() * 5.0f)),
                                 {1.0f, 0.0f, 0.0f});
        cube.model = glm::rotate(cube.model,
                                 static_cast<float>(glm::radians(glfwGetTime() * 4.0f)),
                                 {0.0f, 1.0f, 0.0f});
        DrawMesh(regular_shader, cube, view);

        big_guy.model = glm::mat4(1.0f);
        big_guy.model = glm::translate(big_guy.model, {-5.0f, -4.0f, -14.5f});
        big_guy.model = glm::rotate(big_guy.model, glm::radians(30.0f), {0.0f, 1.0f, 0.0f});
        DrawMesh(regular_shader, big_guy, view);

        monster_frog.model = glm::mat4(1.0f);
        monster_frog.model = glm::translate(monster_frog.model, {20.5f, -1.0f, -16.5f});
        monster_frog.model = glm::scale(monster_frog.model, glm::vec3(0.6f));
        monster_frog.model = glm::rotate(monster_frog.model, glm::radians(-50.0f), {0.0f, 1.0f, 0.0f});
        DrawMesh(regular_shader, monster_frog, view);

        if (point_light_enabled) {
            // Light cube.
            glUseProgram(light_shader);

            for (std::size_t i = 0; i < point_lights.size(); ++i) {
                cube.model = glm::mat4(1.0f);
                cube.model = glm::translate(cube.model, point_lights[i].position);
                cube.model = glm::scale(cube.model, glm::vec3(0.4f));
                DrawMesh(light_shader, cube, view);
            }
        }

        // B-Spline Patches
        glUseProgram(tess_shader);

        glUniform1f(glGetUniformLocation(tess_shader, "tess_level"), 1.0f);
        quad_cube.model = glm::mat4(1.0f);
        quad_cube.model = glm::scale(quad_cube.model, glm::vec3(1.5f));
        quad_cube.model = glm::translate(quad_cube.model, {-2.0f, 0.0f, 0.0f});
        DrawMesh(tess_shader, quad_cube, view, GL_PATCHES);

        glUniform1f(glGetUniformLocation(tess_shader, "tess_level"), 2.0f);
        quad_cube.model = glm::mat4(1.0f);
        quad_cube.model = glm::scale(quad_cube.model, glm::vec3(1.5f));
        quad_cube.model = glm::translate(quad_cube.model, {-1.0f, 0.0f, 0.0f});
        DrawMesh(tess_shader, quad_cube, view, GL_PATCHES);

        glUniform1f(glGetUniformLocation(tess_shader, "tess_level"), 3.0f);
        quad_cube.model = glm::mat4(1.0f);
        quad_cube.model = glm::scale(quad_cube.model, glm::vec3(1.5f));
        quad_cube.model = glm::translate(quad_cube.model, {0.0f, 0.0f, 0.0f});
        DrawMesh(tess_shader, quad_cube, view, GL_PATCHES);

        glUniform1f(glGetUniformLocation(tess_shader, "tess_level"), 4.0f);
        quad_cube.model = glm::mat4(1.0f);
        quad_cube.model = glm::scale(quad_cube.model, glm::vec3(1.5f));
        quad_cube.model = glm::translate(quad_cube.model, {1.0f, 0.0f, 0.0f});
        DrawMesh(tess_shader, quad_cube, view, GL_PATCHES);

        glUniform1f(glGetUniformLocation(tess_shader, "tess_level"), 16.0f);
        quad_cube.model = glm::mat4(1.0f);
        quad_cube.model = glm::scale(quad_cube.model, glm::vec3(1.5f));
        quad_cube.model = glm::translate(quad_cube.model, {2.0f, 0.0f, 0.0f});
        DrawMesh(tess_shader, quad_cube, view, GL_PATCHES);

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }
}

void DrawMesh(const GLuint shader_id, const Mesh& mesh, const glm::mat4& view_matrix, const GLenum mesh_type) {
    SetMaterial(shader_id, mesh.mat);

    glBindVertexArray(mesh.vao);

    glUniformMatrix4fv(glGetUniformLocation(shader_id, "model"), 1, GL_FALSE, glm::value_ptr(mesh.model));

    GLint normal_mat_loc = glGetUniformLocation(shader_id, "normal_mat");
    glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(view_matrix * mesh.model)));
    glUniformMatrix3fv(normal_mat_loc, 1, GL_FALSE, glm::value_ptr(normal_matrix));

    glDrawArrays(mesh_type, 0, mesh.vertices.size());
}

GLuint CreateUBO(const std::size_t buffer_size, const GLenum access_type) {
    GLuint ubo;
    glGenBuffers(1, &ubo);

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, buffer_size, nullptr, access_type);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return ubo;
}

GLuint CreateSSBO(const std::size_t buffer_size, const GLenum access_type) {
    GLuint ssbo;
    glGenBuffers(1, &ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size, nullptr, access_type);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return ssbo;
}

GLuint SetMatricesUBO(float aspect) {
    constexpr int bind_index = 0;
    constexpr std::size_t ubo_size = 2 * sizeof(glm::mat4);
    GLuint matrices_UBO = CreateUBO(ubo_size, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, bind_index, matrices_UBO, 0, ubo_size);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(proj));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return matrices_UBO;
}

GLuint SetTessellationUBO() {
    constexpr int bind_index = 2;
    constexpr std::size_t ubo_size = 2 * sizeof(glm::mat4);
    GLuint tess_UBO = CreateUBO(ubo_size);
    glBindBufferRange(GL_UNIFORM_BUFFER, bind_index, tess_UBO, 0, ubo_size);

    glm::mat4 position_coefs{-1.0f,  3.0f, -3.0f, 1.0f,
                              3.0f, -6.0f,  0.0f, 4.0f,
                             -3.0f,  3.0f,  3.0f, 1.0f,
                              1.0f,  0.0f,  0.0f, 0.0f};
    position_coefs *= 1.0f/6.0f;

    // Actually a mat4x3 but glsl needs padding at the end of each column.
    glm::mat4 tangent_coefs{-1.0f,  2.0f, -1.0f, 0.0f,
                             3.0f, -4.0f,  0.0f, 0.0f,
                            -3.0f,  2.0f,  1.0f, 0.0f,
                             1.0f,  0.0f,  0.0f, 0.0f};
    tangent_coefs *= 0.5f;

    glBindBuffer(GL_UNIFORM_BUFFER, tess_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(position_coefs));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(tangent_coefs));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return tess_UBO;
}

GLuint SetLightsUBO(bool dir_enable, bool point_enable,
                    const DirLight& dir_light, const std::vector<PointLight>& point_lights) {
    constexpr int bind_index = 1;
    std::size_t ubo_size = 80 + point_lights.size() * 80;
    GLuint lights_UBO = CreateUBO(ubo_size, GL_DYNAMIC_DRAW);
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

std::vector<glm::vec3> PatchQuads() {
    return {{-0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f}, // center
            {-0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f},
            { 0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f},
            { 0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f},

            { 0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f}, // right
            { 0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f},
            { 0.75f,  0.00f,  0.25f}, { 0.0f,  1.0f,  0.0f},
            { 0.75f,  0.00f, -0.25f}, { 0.0f,  1.0f,  0.0f},

            {-0.75f,  0.00f, -0.25f}, { 0.0f,  1.0f,  0.0f}, // left
            {-0.75f,  0.00f,  0.25f}, { 0.0f,  1.0f,  0.0f},
            {-0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f},
            {-0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f},

            {-0.25f,  0.00f, -0.75f}, { 0.0f,  1.0f,  0.0f}, // top
            {-0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f},
            { 0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f},
            { 0.25f,  0.00f, -0.75f}, { 0.0f,  1.0f,  0.0f},

            {-0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f}, // bottom
            {-0.25f,  0.00f,  0.75f}, { 0.0f,  1.0f,  0.0f},
            { 0.25f,  0.00f,  0.75f}, { 0.0f,  1.0f,  0.0f},
            { 0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f},

            { 0.25f,  0.00f, -0.75f}, { 0.0f,  1.0f,  0.0f}, // top right
            { 0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f},
            { 0.75f,  0.00f, -0.25f}, { 0.0f,  1.0f,  0.0f},
            { 0.75f, -0.25f, -0.75f}, { 0.0f,  1.0f,  0.0f},

            {-0.75f, -0.25f, -0.75f}, { 0.0f,  1.0f,  0.0f}, // top left
            {-0.75f,  0.00f, -0.25f}, { 0.0f,  1.0f,  0.0f},
            {-0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f},
            {-0.25f,  0.00f, -0.75f}, { 0.0f,  1.0f,  0.0f},

            { 0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f}, // bottom right
            { 0.25f,  0.00f,  0.75f}, { 0.0f,  1.0f,  0.0f},
            { 0.75f, -0.25f,  0.75f}, { 0.0f,  1.0f,  0.0f},
            { 0.75f,  0.00f,  0.25f}, { 0.0f,  1.0f,  0.0f},

            {-0.75f,  0.00f,  0.25f}, { 0.0f,  1.0f,  0.0f}, // bottom left
            {-0.75f, -0.25f,  0.75f}, { 0.0f,  1.0f,  0.0f},
            {-0.25f,  0.00f,  0.75f}, { 0.0f,  1.0f,  0.0f},
            {-0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f}};
}

std::vector<glm::vec3> PatchVerts() {
    return {{-0.75f, -0.25f,  0.75f}, { 0.0f,  1.0f,  0.0f}, // bottom left
            {-0.25f,  0.00f,  0.75f}, { 0.0f,  1.0f,  0.0f}, // bottom
            { 0.25f,  0.00f,  0.75f}, { 0.0f,  1.0f,  0.0f}, // bottom
            { 0.75f, -0.25f,  0.75f}, { 0.0f,  1.0f,  0.0f}, // bottom right

            {-0.75f,  0.00f,  0.25f}, { 0.0f,  1.0f,  0.0f}, // left
            {-0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f}, // center
            { 0.25f,  0.25f,  0.25f}, { 0.0f,  1.0f,  0.0f}, // center
            { 0.75f,  0.00f,  0.25f}, { 0.0f,  1.0f,  0.0f}, // right

            {-0.75f,  0.00f, -0.25f}, { 0.0f,  1.0f,  0.0f}, // left
            {-0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f}, // center
            { 0.25f,  0.25f, -0.25f}, { 0.0f,  1.0f,  0.0f}, // center
            { 0.75f,  0.00f, -0.25f}, { 0.0f,  1.0f,  0.0f}, // right

            {-0.75f, -0.25f, -0.75f}, { 0.0f,  1.0f,  0.0f}, // top left
            {-0.25f,  0.00f, -0.75f}, { 0.0f,  1.0f,  0.0f}, // top
            { 0.25f,  0.00f, -0.75f}, { 0.0f,  1.0f,  0.0f}, // top
            { 0.75f, -0.25f, -0.75f}, { 0.0f,  1.0f,  0.0f}, // top right
    };
}

} // End namespace Renderer.
