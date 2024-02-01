#version 450

#extension GL_EXT_scalar_block_layout: require

layout (location = 0) in vec2 texcoord;

layout (location = 0) out vec4 out_frag;

void main() {
    vec3 color = vec3(texcoord.xy, 0.5);
    out_frag = vec4(color, 1.0);
}
