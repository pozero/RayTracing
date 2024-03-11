#version 460

vec2 vertices[3] = vec2[3](
    vec2( 3.0, -1.0), 
    vec2(-1.0, -1.0), 
    vec2(-1.0,  3.0));

layout (location = 0) out vec2 texcoord;

void main() {
    gl_Position = vec4(vertices[gl_VertexIndex], 1.0, 1.0);
    texcoord = 0.5 * gl_Position.xy + vec2(0.5);
}
