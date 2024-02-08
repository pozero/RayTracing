#include "renderable.h"

glsl_material create_lambertian(glm::vec3 const& albedo) {
    return glsl_material{material_type::lambertian, albedo, 0.0f, 0.0f};
}

glsl_material create_metal(glm::vec3 const& albedo, float fuzz) {
    return glsl_material{material_type::metal, albedo, fuzz, 0.0f};
}

glsl_material create_dielectric(float refraction_index) {
    return glsl_material{material_type::dielectric, {}, 0.0f, refraction_index};
}

glsl_sphere create_sphere(
    glm::vec3 const& center, float radius, glsl_material const& material) {
    return glsl_sphere{
        .center = center,
        .radius = radius,
        .material = material,
    };
}
