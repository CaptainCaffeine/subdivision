#version 430 core

layout(quads, equal_spacing, ccw) in;

layout (std140) uniform TessMatrices {
    mat4 bspline_position;
    mat4x3 bspline_tangent;
};

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

vec4 BSplinePatchPosition(vec4 u_vec, vec4 v_vec, mat4 points_x, mat4 points_y, mat4 points_z) {
    // Compute & store the B-Spline basis functions to avoid repeating the matrix multiplication.
    vec4 bspline_position_u = u_vec * bspline_position;
    vec4 bspline_position_v = transpose(bspline_position) * v_vec;

    vec4 position;
    position.x = dot(bspline_position_u, points_x * bspline_position_v);
    position.y = dot(bspline_position_u, points_y * bspline_position_v);
    position.z = dot(bspline_position_u, points_z * bspline_position_v);
    position.w = 1.0f;

    return position;
}

vec3 BSplinePatchNormal(vec4 u_vec4, vec4 v_vec4, mat4 points_x, mat4 points_y, mat4 points_z) {
    // No u^3 term in the derivative matrix.
    vec3 u_vec3 = u_vec4.yzw;
    vec3 v_vec3 = v_vec4.yzw;

    // Compute & store the B-Spline basis functions and derivatives to avoid repeating the matrix multiplication.
    vec4 bspline_position_u = u_vec4 * bspline_position;
    vec4 bspline_position_v = transpose(bspline_position) * v_vec4;
    vec4 bspline_tangent_u = u_vec3 * bspline_tangent;
    vec4 bspline_tangent_v = transpose(bspline_tangent) * v_vec3;

    vec3 u_tangent;
    u_tangent.x = dot(bspline_tangent_u, points_x * bspline_position_v);
    u_tangent.y = dot(bspline_tangent_u, points_y * bspline_position_v);
    u_tangent.z = dot(bspline_tangent_u, points_z * bspline_position_v);

    vec3 v_tangent;
    v_tangent.x = dot(bspline_position_u * points_x, bspline_tangent_v);
    v_tangent.y = dot(bspline_position_u * points_y, bspline_tangent_v);
    v_tangent.z = dot(bspline_position_u * points_z, bspline_tangent_v);

    return cross(u_tangent, v_tangent);
}

void main() {
    // Control point matrices.
    mat4 points_x = mat4(tes_in[ 0].position.x, tes_in[ 1].position.x, tes_in[ 2].position.x, tes_in[ 3].position.x,
                         tes_in[ 4].position.x, tes_in[ 5].position.x, tes_in[ 6].position.x, tes_in[ 7].position.x,
                         tes_in[ 8].position.x, tes_in[ 9].position.x, tes_in[10].position.x, tes_in[11].position.x,
                         tes_in[12].position.x, tes_in[13].position.x, tes_in[14].position.x, tes_in[15].position.x);

    mat4 points_y = mat4(tes_in[ 0].position.y, tes_in[ 1].position.y, tes_in[ 2].position.y, tes_in[ 3].position.y,
                         tes_in[ 4].position.y, tes_in[ 5].position.y, tes_in[ 6].position.y, tes_in[ 7].position.y,
                         tes_in[ 8].position.y, tes_in[ 9].position.y, tes_in[10].position.y, tes_in[11].position.y,
                         tes_in[12].position.y, tes_in[13].position.y, tes_in[14].position.y, tes_in[15].position.y);

    mat4 points_z = mat4(tes_in[ 0].position.z, tes_in[ 1].position.z, tes_in[ 2].position.z, tes_in[ 3].position.z,
                         tes_in[ 4].position.z, tes_in[ 5].position.z, tes_in[ 6].position.z, tes_in[ 7].position.z,
                         tes_in[ 8].position.z, tes_in[ 9].position.z, tes_in[10].position.z, tes_in[11].position.z,
                         tes_in[12].position.z, tes_in[13].position.z, tes_in[14].position.z, tes_in[15].position.z);

    vec4 u_vec = vec4(pow(gl_TessCoord.x, 3.0f), pow(gl_TessCoord.x, 2.0f), gl_TessCoord.x, 1.0f);
    vec4 v_vec = vec4(pow(gl_TessCoord.y, 3.0f), pow(gl_TessCoord.y, 2.0f), gl_TessCoord.y, 1.0f);

    vec4 position = BSplinePatchPosition(u_vec, v_vec, points_x, points_y, points_z);
    vec3 normal = BSplinePatchNormal(u_vec, v_vec, points_x, points_y, points_z);

    gl_Position = proj * view * model * position;
    tes_out.frag_pos = vec3(view * model * position);
    tes_out.normal = normal_mat * normalize(normal);
}
