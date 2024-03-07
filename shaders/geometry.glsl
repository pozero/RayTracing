struct sphere_t {
    vec3 center;
    float radius;
    material_t material;
};

struct triangel_vertex_t {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec2 albedo_uv;
    vec2 normal_uv;
};

struct triangle_t {
    uint a;
    uint b;
    uint c;
    material_t material;
};

