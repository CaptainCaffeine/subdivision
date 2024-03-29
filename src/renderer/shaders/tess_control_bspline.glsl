#version 430 core

layout (vertices = 16) out;

in VertexData {
    vec3 position;
    vec3 normal;
} tcs_in[];

out VertexData {
    vec3 position;
    vec3 normal;
} tcs_out[];

uniform float tess_level;

void main() {
    tcs_out[gl_InvocationID].position = tcs_in[gl_InvocationID].position;
    tcs_out[gl_InvocationID].normal = tcs_in[gl_InvocationID].normal;

    gl_TessLevelOuter[0] = tess_level;
    gl_TessLevelOuter[1] = tess_level;
    gl_TessLevelOuter[2] = tess_level;
    gl_TessLevelOuter[3] = tess_level;
    gl_TessLevelInner[0] = tess_level;
    gl_TessLevelInner[1] = tess_level;
}
