#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out VertexData {
    vec3 position;
    vec3 normal;
} vs_out;

void main() {
    gl_Position = vec4(position, 1.0);
    vs_out.position = position;
    vs_out.normal = normal;
}
