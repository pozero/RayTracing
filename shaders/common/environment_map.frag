#version 460

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 out_frag;

#include "../common/to_ldr.glsl"

layout(set = 0, binding = 0) uniform samplerCube environment_map;

void main() {
    const vec3 color = textureLod(environment_map, position, 1.1).rgb;
    out_frag = vec4(gamma_correct(tone_mapping(color)), 1.0);
}
