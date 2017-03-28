#version 430 core

layout(triangles, equal_spacing, ccw) in;

layout (std140) uniform Matrices {
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
    vec3 position = tes_in[0].position * vec3(gl_TessCoord.x) +
                    tes_in[1].position * vec3(gl_TessCoord.y) +
                    tes_in[2].position * vec3(gl_TessCoord.z);
    vec3 normal = tes_in[0].normal * vec3(gl_TessCoord.x) +
                  tes_in[1].normal * vec3(gl_TessCoord.y) +
                  tes_in[2].normal * vec3(gl_TessCoord.z);

    normal = normalize(normal);

    gl_Position = proj * view * model * vec4(position, 1.0);
    tes_out.frag_pos = vec3(view * model * vec4(position, 1.0));
    tes_out.normal = normal_mat * normal;
}
