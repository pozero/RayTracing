#define PI 3.1415926535

#define MATERIAL_LAMBERTIAN 0
#define MATERIAL_METAL 1
#define MATERIAL_DIELECTRIC 2

struct material_t {
    vec4 albedo;
    float fuzz;
    float refraction_index;
    int albedo_texture;
    int type;
};

material_t empty_material() {
    return material_t(vec4(0.0, 0.0, 0.0, 1.0), 0.0, 0.0, -1, -1);
}

