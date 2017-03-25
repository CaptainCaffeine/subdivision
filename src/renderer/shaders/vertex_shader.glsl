#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 in_normal;

layout (std140) uniform Matrices {
    mat4 proj;
    mat4 view;
};

out ShadingData {
    vec3 frag_pos;
    vec3 normal;
} vs_out;

uniform mat4 model;
uniform mat3 normal_mat;

void main() {
    gl_Position = proj * view * model * vec4(position, 1.0);
    vs_out.frag_pos = vec3(view * model * vec4(position, 1.0));
    vs_out.normal = normal_mat * in_normal;
}
