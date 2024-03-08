struct sphere_t {
    vec3 center;
    float radius;
    material_t material;
};

struct triangel_vertex_t {
    vec4 position;
    vec4 normal;
    vec4 tangent;
    vec2 albedo_uv;
    vec2 normal_uv;
};

struct triangle_t {
    uint a;
    uint b;
    uint c;
    uint material;
};

