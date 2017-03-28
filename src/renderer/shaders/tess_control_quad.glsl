#version 430 core

layout (vertices = 4) out;

in VertexData {
    vec3 position;
    vec3 normal;
} tcs_in[];

out VertexData {
    vec3 position;
    vec3 normal;
} tcs_out[];

void main() {
    tcs_out[gl_InvocationID].position = tcs_in[gl_InvocationID].position;
    tcs_out[gl_InvocationID].normal = tcs_in[gl_InvocationID].normal;

    gl_TessLevelOuter[0] = 4.0f;
    gl_TessLevelOuter[1] = 4.0f;
    gl_TessLevelOuter[2] = 4.0f;
    gl_TessLevelOuter[3] = 4.0f;
    gl_TessLevelInner[0] = 4.0f;
    gl_TessLevelInner[1] = 4.0f;
}
