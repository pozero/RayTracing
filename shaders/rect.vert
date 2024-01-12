#version 460 core

vec2 vertices[3] = vec2[3](
    vec2(-1,-1), 
    vec2( 3,-1), 
    vec2(-1, 3));

out vec2 texcoord;

void main() {
    gl_Position = vec4(vertices[gl_VertexID],0,1);
    texcoord = 0.5 * gl_Position.xy + vec2(0.5);
}
