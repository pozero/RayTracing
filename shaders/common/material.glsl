struct material_t {
    vec3 albedo;
    float albedo_tex;

    float metallic;
    float subsurface;
    float specular;
    float roughness;

    float specular_tint;
    float anisotropic;
    float sheen;
    float sheen_tint;

    float clearcoat;
    float clearcoat_gloss;
    // padding
    // padding
};
