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

glsl_aabb create_empty_aabb() {
    glm::vec2 const empty_interval{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::min(),
    };
    return glsl_aabb{
        empty_interval,
        empty_interval,
        empty_interval,
    };
}

float get_aabb_volume(glsl_aabb const& aabb) {
    return (aabb.x_interval[1] - aabb.x_interval[0]) *
           (aabb.y_interval[1] - aabb.y_interval[0]) *
           (aabb.z_interval[1] - aabb.z_interval[0]);
}

inline glsl_sphere create_sphere(
    glm::vec3 const& center, float radius, glsl_material const& material) {
    glm::vec2 const mu{-0.5f, 0.5f};
    glm::vec2 const x_interval =
        glm::vec2{center[0] - glm::abs(radius), center[0] + glm::abs(radius)} +
        mu;
    glm::vec2 const y_interval =
        glm::vec2{center[1] - glm::abs(radius), center[1] + glm::abs(radius)} +
        mu;
    glm::vec2 const z_interval =
        glm::vec2{center[2] - glm::abs(radius), center[2] + glm::abs(radius)} +
        mu;
    glsl_aabb const aabb{
        x_interval,
        y_interval,
        z_interval,
    };
    return glsl_sphere{
        .center = center,
        .radius = radius,
        .material = material,
        .aabb = aabb,
    };
}
