#version 460

#extension GL_EXT_scalar_block_layout: require

#include "../common/to_ldr.glsl"

layout (location = 0) in vec2 texcoord;

layout (location = 0) out vec4 out_frag;

layout(set = 0, binding = 0) uniform sampler2D frame[2];

layout(std430, push_constant) uniform push_constant {
    float frame_scalar;
    uint preview;
};

void main() {
    const vec3 color = 
        gamma_correct(
            tone_mapping(frame_scalar * texture(frame[preview], texcoord.xy).rgb));
    out_frag = vec4(color, 1.0);
}
