#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 in_normal;

out VertexData {
    vec3 normal;
} vs_out;

void main() {
    gl_Position = vec4(position, 1.0);
    vs_out.normal = in_normal;
}
