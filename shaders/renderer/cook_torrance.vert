#version 460

#extension GL_EXT_scalar_block_layout: require

#include "../common/material.glsl"
#include "../common/geometry.glsl"

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 uv;
layout (location = 3) out flat uint material;

layout(push_constant, std430) uniform push_constants {
    mat4 proj_view_mat;
};

layout(std430, set = 0, binding = 0) readonly buffer triangle_geometries_vertex {
    triangel_vertex_t triangle_vertices[];
};

layout(std430, set = 0, binding = 1) readonly buffer triangle_geometries_face {
    triangle_t triangles[];
};

void main() {
    const uint triangle_index = gl_VertexIndex / 3;
    const uint vertex_index_index = gl_VertexIndex % 3;
    const triangle_t triangle = triangles[triangle_index];
    const uint vertex_indices[3] = uint[3](triangle.a,
                                           triangle.b,
                                           triangle.c);
    const triangel_vertex_t vertex = triangle_vertices[vertex_indices[vertex_index_index]];
    gl_Position = proj_view_mat * vertex.position;
    position = vertex.position.xyz;
    normal = vertex.normal.xyz;
    uv = vertex.albedo_uv;
    material = triangle.material;
}
