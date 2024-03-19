#version 460

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_nonuniform_qualifier: require

#include "../common/material.glsl"
#include "../common/to_ldr.glsl"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in flat uint in_material;

layout(location = 0) out vec4 out_frag;

layout(std430, set = 1, binding = 0) readonly buffer MATERIAL {
    material_t materials[];
};

layout(set = 1, binding = 1) uniform sampler2D textures[];

void main() {
    out_frag = vec4(1.0, 1.0, 1.0, 1.0);
}
