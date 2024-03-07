#include "renderable.h"
#include "texture.h"

glsl_material create_lambertian(glm::vec3 const& albedo) {
    return glsl_material{material_type::lambertian, albedo, 0.0f, 0.0f, -1};
}

glsl_material create_lambertian(std::string_view texture_path,
    std::vector<struct texture_data>& texture_datas) {
    texture_datas.emplace_back(get_texture_data(texture_path));
    return glsl_material{material_type::lambertian, {}, 0.0f, 0.0f,
        (int) texture_datas.size() - 1};
}

glsl_material create_metal(glm::vec3 const& albedo, float fuzz) {
    return glsl_material{material_type::metal, albedo, fuzz, 0.0f, -1};
}

glsl_material create_dielectric(float refraction_index) {
    return glsl_material{
        material_type::dielectric, {}, 0.0f, refraction_index, -1};
}

glsl_sphere create_sphere(
    glm::vec3 const& center, float radius, glsl_material const& material) {
    return glsl_sphere{
        .center = center,
        .radius = radius,
        .material = material,
    };
}

void add_quad(triangle_mesh& mesh, glm::vec3 const& upper_left,
    glm::vec3 const& u, glm::vec3 const& v, glsl_material const& material) {
    glm::vec3 const normal = glm::normalize(glm::cross(u, v));
    glsl_triangle_vertex const v0{
        .position = upper_left,
        .normal = normal,
        .tangent = {},
        .albedo_uv = glm::vec2{0.0f, 0.0f},
        .normal_uv = {},
    };
    glsl_triangle_vertex const v1{
        .position = upper_left + u,
        .normal = normal,
        .tangent = {},
        .albedo_uv = glm::vec2{1.0f, 0.0f},
        .normal_uv = {},
    };
    glsl_triangle_vertex const v2{
        .position = upper_left + v,
        .normal = normal,
        .tangent = {},
        .albedo_uv = glm::vec2{0.0f, 1.0f},
        .normal_uv = {},
    };
    glsl_triangle_vertex const v3{
        .position = upper_left + u + v,
        .normal = normal,
        .tangent = {},
        .albedo_uv = glm::vec2{1.0f, 1.0f},
        .normal_uv = {},
    };
    uint32_t const vertex_base_index = (uint32_t) mesh.vertices.size();
    glsl_triangle const t0{
        .a = vertex_base_index + 0,
        .b = vertex_base_index + 1,
        .c = vertex_base_index + 2,
        .material = material,
    };
    glsl_triangle const t1{
        .a = vertex_base_index + 2,
        .b = vertex_base_index + 1,
        .c = vertex_base_index + 3,
        .material = material,
    };
    mesh.vertices.push_back(v0);
    mesh.vertices.push_back(v1);
    mesh.vertices.push_back(v2);
    mesh.vertices.push_back(v3);
    mesh.triangles.push_back(t0);
    mesh.triangles.push_back(t1);
}
