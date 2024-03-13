#version 460

#extension GL_EXT_scalar_block_layout: require

vec3 vertices[36] = vec3[36](
     // back face
     vec3(-1.0, -1.0, -1.0), // bottom-left
     vec3( 1.0,  1.0, -1.0), // top-right
     vec3( 1.0, -1.0, -1.0), // bottom-right         
     vec3( 1.0,  1.0, -1.0), // top-right
     vec3(-1.0, -1.0, -1.0), // bottom-left
     vec3(-1.0,  1.0, -1.0), // top-left
     // front face
     vec3(-1.0, -1.0,  1.0), // bottom-left
     vec3( 1.0, -1.0,  1.0), // bottom-right
     vec3( 1.0,  1.0,  1.0), // top-right
     vec3( 1.0,  1.0,  1.0), // top-right
     vec3(-1.0,  1.0,  1.0), // top-left
     vec3(-1.0, -1.0,  1.0), // bottom-left
     // left face
     vec3(-1.0,  1.0,  1.0), // top-right
     vec3(-1.0,  1.0, -1.0), // top-left
     vec3(-1.0, -1.0, -1.0), // bottom-left
     vec3(-1.0, -1.0, -1.0), // bottom-left
     vec3(-1.0, -1.0,  1.0), // bottom-right
     vec3(-1.0,  1.0,  1.0), // top-right
     // right face
     vec3( 1.0,  1.0,  1.0), // top-left
     vec3( 1.0, -1.0, -1.0), // bottom-right
     vec3( 1.0,  1.0, -1.0), // top-right         
     vec3( 1.0, -1.0, -1.0), // bottom-right
     vec3( 1.0,  1.0,  1.0), // top-left
     vec3( 1.0, -1.0,  1.0), // bottom-left     
     // bottom face
     vec3(-1.0, -1.0, -1.0), // top-right
     vec3( 1.0, -1.0, -1.0), // top-left
     vec3( 1.0, -1.0,  1.0), // bottom-left
     vec3( 1.0, -1.0,  1.0), // bottom-left
     vec3(-1.0, -1.0,  1.0), // bottom-right
     vec3(-1.0, -1.0, -1.0), // top-right
     // top face
     vec3(-1.0,  1.0, -1.0), // top-left
     vec3( 1.0,  1.0 , 1.0), // bottom-right
     vec3( 1.0,  1.0, -1.0), // top-right     
     vec3( 1.0,  1.0,  1.0), // bottom-right
     vec3(-1.0,  1.0, -1.0), // top-left
     vec3(-1.0,  1.0,  1.0)  // bottom-left
);

layout(location = 0) out vec3 position;

layout(push_constant, std430) uniform push_constants {
    mat4 proj_view;
};

void main() {
    const vec3 position_on_cube = vertices[gl_VertexIndex];
    position = position_on_cube;
    const vec4 clipped = proj_view * vec4(position_on_cube, 1.0);
    gl_Position = clipped.xyww;
}
