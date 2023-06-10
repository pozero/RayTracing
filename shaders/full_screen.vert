#version 460

const vec3 full_screen_pos[3] =
    vec3[3](vec3(-1, 1, 1), vec3(3, 1, 1), vec3(-1, -3, 1));

const vec2 full_screen_uv[3] = vec2[3](vec2(0, 0), vec2(2, 0), vec2(0, 2));

layout(location = 0) out vec2 out_uv;

void main() {
    gl_Position = vec4(full_screen_pos[gl_VertexIndex], 1.0);
    out_uv = full_screen_uv[gl_VertexIndex];
}
