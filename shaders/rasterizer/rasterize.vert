#version 460

#extension GL_EXT_scalar_block_layout: require

#include "../common/geometry.glsl"

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec2 out_tex_coord;
layout(location = 2) out flat int out_material;
layout(location = 3) out flat int out_light;

layout(std430, set = 0, binding = 0) readonly buffer VERTICES {
    vec4 packed_vertices[];
};

layout(std430, set = 0, binding = 1) readonly buffer TRANSFORMATION {
    mat4 transformations[];
};

struct instance_t {
    int transform;
    int material;
    int light;
};

layout(std430, set = 0, binding = 2) readonly buffer INSTANCE {
    instance_t instances[];
};

layout(std430, push_constant) uniform PUSH_CONSTANTS {
    mat4 proj_view_mat;
};

void main() {
    const uint inst_idx = gl_InstanceIndex;
    const vertex_t vertex = unpack_vertex(packed_vertices[gl_VertexIndex * 2 + 0], 
                                          packed_vertices[gl_VertexIndex * 2 + 1]);
    const instance_t inst = instances[inst_idx];
    vec4 world_position = vec4(vertex.position, 1.0);
    if (inst.transform >= 0) {
        world_position = transformations[inst.transform] * vec4(vertex.position, 1.0);
    }
    gl_Position = proj_view_mat * world_position;
    out_position = world_position.xyz;
    out_tex_coord = vertex.tex_coord;
    out_material = inst.material;
    out_light = inst.light;
}
