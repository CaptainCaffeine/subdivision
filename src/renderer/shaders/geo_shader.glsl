#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (std140) uniform Matrices {
    mat4 proj;
    mat4 view;
};

in VertexData {
    vec3 position;
    vec3 normal;
} gs_in[];

out ShadingData {
    vec3 frag_pos;
    vec3 normal;
} gs_out;

uniform float time;

uniform mat4 model;
uniform mat3 normal_mat;

vec4 Explode(vec4 position, vec3 normal) {
    vec3 direction = normal * ((sin(time) + 1.0f) / 2.0f);
    return position + vec4(direction, 0.0f);
}

void main() {
    for (int i = 0; i < gs_in.length(); ++i) {
        vec4 translated_pos = Explode(gl_in[i].gl_Position, gs_in[i].normal);
        gl_Position = proj * view * model * translated_pos;
        gs_out.frag_pos = vec3(view * model * translated_pos);
        gs_out.normal = normal_mat * gs_in[i].normal;
        EmitVertex();
    }

    EndPrimitive();
}
