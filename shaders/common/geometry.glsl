struct vertex_t {
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
};

vertex_t unpack_vertex(const in vec4 position_texu,
                       const in vec4 normal_texv) {
    return vertex_t(position_texu.xyz, normal_texv.xyz, vec2(position_texu.w, normal_texv.w));
}
