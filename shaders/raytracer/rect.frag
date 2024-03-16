#version 460

#extension GL_EXT_scalar_block_layout: require

#include "../common/to_ldr.glsl"

layout (location = 0) in vec2 texcoord;

layout (location = 0) out vec4 out_frag;

layout(set = 0, binding = 0) uniform sampler2D frame;

layout(std430, push_constant) uniform push_constant {
    float texture_scalar;
};

void main() {
    const vec3 color = gamma_correct(tone_mapping(texture_scalar * texture(frame, texcoord.xy).rgb));
    out_frag = vec4(color, 1.0);
}
