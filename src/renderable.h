#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/common.hpp"
#pragma clang diagnostic pop

enum class material_type {
    lambertian,
    metal,
    dielectric,
};

struct glsl_material {
    material_type type;
    alignas(sizeof(glm::vec4)) glm::vec3 albedo;
    float fuzz;
    float refraction_index;
};

glsl_material create_lambertian(glm::vec3 const& albedo);
glsl_material create_metal(glm::vec3 const& albedo, float fuzz);
glsl_material create_dielectric(float refraction_index);

struct glsl_aabb {
    glm::vec2 x_interval;
    glm::vec2 y_interval;
    glm::vec2 z_interval;
};

glsl_aabb create_empty_aabb();
float get_aabb_volume(glsl_aabb const& aabb);

struct glsl_sphere {
    glm::vec3 center;
    float radius;
    glsl_material material;
    glsl_aabb aabb;
};
