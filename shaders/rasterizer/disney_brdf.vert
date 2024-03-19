#version 460

#extension GL_EXT_scalar_block_layout: require

#include "../common/geometry.glsl"

layout(location = 0) out vec3 position;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 tex_coord;
layout(location = 3) out flat uint material;

layout(std430, set = 0, binding = 0) readonly buffer VERTICES {
    vec4 packed_vertices[];
};

layout(std430, set = 0, binding = 1) readonly buffer TRANSFORMATION {
    mat4 transformations[];
};

layout(std430, set = 0, binding = 2) readonly buffer NORMAL_TRANSFORMATION {
    mat3 normal_transformations[];
};

layout(std430, set = 0, binding = 3) readonly buffer INSTANCE_MATERIAL_INDEX {
    uint material_indices[];
};

layout(std430, push_constant) uniform PUSH_CONSTANTS {
    mat4 proj_view_mat;
};

void main() {
    const uint instance_idx = gl_InstanceIndex;
    const vertex_t vertex = unpack_vertex(packed_vertices[gl_VertexIndex * 2 + 0], 
                                          packed_vertices[gl_VertexIndex * 2 + 1]);
    const vec4 world_position = transformations[instance_idx] * vec4(vertex.position, 1.0);
    gl_Position = proj_view_mat * world_position;
    position = world_position.xyz;
    normal = normal_transformations[instance_idx] * vertex.normal;
    tex_coord = vertex.tex_coord;
    material = material_indices[instance_idx];
}
