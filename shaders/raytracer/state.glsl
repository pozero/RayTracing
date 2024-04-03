struct state_t {
    vec3 hit_position;
    vec3 hit_normal;
    vec3 hit_tangent;
    vec3 hit_bitangent;
    vec2 hit_uv;
    float hit_t;
    int inst_material;
    int inst_medium;
    int inst_light;
    bool front_face;
};

vec3 get_front_face_normal(const in state_t state) {
    return state.front_face ? state.hit_normal : -state.hit_normal;
}

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
    float ax;
    float ay;
    float sheen;
    float sheen_tint;
    float clearcoat;
    float clearcoat_gloss;
};

surface_info_t empty_surface_info() {
    return surface_info_t(
        vec3(0.0),
        vec3(0.0),
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    );
}

struct light_sample_t {
    vec3 intensity;
    float pdf;
    vec3 wi;
    int type;
};
