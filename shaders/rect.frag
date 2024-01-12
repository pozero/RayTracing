#version 460 core

in vec2 texcoord;

out vec4 out_frag;

uniform sampler2D frame;

void main() {
    vec3 color = texture(frame, texcoord).rgb;
    out_frag = vec4(color, 1.0);
}
