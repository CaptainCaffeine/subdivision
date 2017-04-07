#version 430 core

layout(quads, equal_spacing, ccw) in;

layout (std140, binding = 0) uniform Matrices {
    mat4 proj;
    mat4 view;
};

uniform mat4 model;
uniform mat3 normal_mat;

in VertexData {
    vec3 position;
    vec3 normal;
} tes_in[];

out ShadingData {
    vec3 frag_pos;
    vec3 normal;
} tes_out;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec3 a = mix(tes_in[0].position, tes_in[1].position, u);
    vec3 b = mix(tes_in[3].position, tes_in[2].position, u);
    vec3 position = mix(a, b, v);

//    a = mix(tes_in[0].normal, tes_in[1].normal, u);
//    b = mix(tes_in[3].normal, tes_in[2].normal, u);
//    vec3 normal = mix(a, b, v);

    // Flat shading.
    vec3 normal = normalize(cross(tes_in[1].position - tes_in[0].position, tes_in[3].position - tes_in[0].position));

    gl_Position = proj * view * model * vec4(position, 1.0);
    tes_out.frag_pos = vec3(view * model * vec4(position, 1.0));
    tes_out.normal = normal_mat * normal;
}
