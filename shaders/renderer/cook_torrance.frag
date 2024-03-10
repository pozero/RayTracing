#version 460

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in flat uint material;

layout (location = 0) out vec4 out_frag;

void main() {
    out_frag = vec4(0.0, 0.0, 0.0, 1.0);
}
