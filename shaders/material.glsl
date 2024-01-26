#define PI 3.1415926535

#define MATERIAL_LAMBERTIAN 0
#define MATERIAL_METAL 1
#define MATERIAL_DIELECTRIC 2

struct material_t {
    int type;
    vec3 albedo;
    float fuzz;
    float refraction_index;
};

material_t empty_material() {
    return material_t(0, vec3(0.0, 0.0, 0.0), 0.0, 0.0);
}

