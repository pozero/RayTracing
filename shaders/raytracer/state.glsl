struct state_t {
    vec3 radiance;
    vec3 throughput;
    vec3 ray_origin;
    vec3 ray_direction;
    vec3 hit_position;
    vec3 hit_normal;
    vec3 hit_tangent;
    vec3 hit_bitangent;
    vec2 hit_uv;
    float hit_t;
    int inst_material;
    // doesn't support stack of mediums now
    int inst_medium;
    int inst_light;
    bool front_face;
};

struct surface_info_t {
    vec3 albedo;
    vec3 emission;
    float metallic;
    float spec_trans;
    float eta;
    float ior;
    float subsurface;
    float roughness;
    float specular_tint;
    float anisotropic;
    float sheen;
    float sheen_tint;
    float clearcoat;
    float clearcoat_gloss;
};

// struct scatter_sample {
// };
// 
// struct light_sample {
// };
