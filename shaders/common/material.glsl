struct material_t {
    vec3 albedo;
    int albedo_tex;

    vec3 emission;
    int emission_tex;

    float metallic;
    float spec_trans;
    float ior;
    float subsurface;

    float roughness;
    float specular_tint;
    float anisotropic;
    float sheen;

    float sheen_tint;
    float clearcoat;
    float clearcoat_gloss;
    int normal_tex;

    int metallic_roughness_tex;
};

#define MEDIUM_ABSORPTION   0
#define MEDIUM_EMISSION     1
#define MEDIUM_SCATTERING   2

struct medium_t {
    vec3 color;
    float density;
    float anisotropic;
    int type;
};
