#version 450

layout (location = 0) in vec2 texcoord;

layout (location = 0) out vec4 out_frag;

layout (binding = 0) uniform sampler2D frame;

void main() {
    vec3 color = texture(frame, texcoord).rgb;
    out_frag = vec4(color, 1.0);
}
