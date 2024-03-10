#version 460

#extension GL_EXT_scalar_block_layout: require

layout (location = 0) in vec2 texcoord;

layout (location = 0) out vec4 out_frag;

layout(set = 0, binding = 0) uniform sampler2D frame;

layout(std430, set = 0, binding = 1) uniform camera {
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    vec3 upper_left_pixel;
    vec3 camera_position;
    float accumulated_scalar;
};

vec3 gamma_correct(const in vec3 value) {
    return pow(value / (value + vec3(1.0)), vec3(1.0 / 2.2));
}

void main() {
    const vec3 color = gamma_correct(accumulated_scalar * texture(frame, texcoord.xy).rgb);
    out_frag = vec4(color, 1.0);
}
