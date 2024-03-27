#version 460

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_nonuniform_qualifier: require

#include "../common/material.glsl"
#include "../common/light.glsl"
#include "../common/to_ldr.glsl"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in flat int in_material;
layout(location = 3) in flat int in_light;

layout(location = 0) out vec4 out_frag;

layout(std430, set = 1, binding = 0) readonly buffer MATERIAL {
    material_t materials[];
};

layout(std430, set = 1, binding = 1) readonly buffer LIGHT {
    light_t lights[];
};

layout(set = 1, binding = 2) uniform sampler2D textures[];

void main() {
    // if (in_material >= 0) {
    //     const material_t mat = materials[in_material];
    //     if (mat.albedo_tex >= 0) {
    //         out_frag = vec4(mat.albedo * texture(nonuniformEXT(textures[mat.albedo_tex]), in_tex_coord).rgb, 1.0);
    //     } else {
    //         out_frag = vec4(mat.albedo, 1.0);
    //     }
    // } else /* in_light >= 0 */ {
    //     const light_t l = lights[in_light];
    //     if (l.emission_tex >= 0) {
    //         out_frag = vec4(l.intensity * texture(nonuniformEXT(textures[l.emission_tex]), in_tex_coord).rgb, 1.0);
    //     } else {
    //         out_frag = vec4(l.intensity, 1.0);
    //     }
    // }
    out_frag = vec4(1.0, 1.0, 1.0, 1.0);
}
