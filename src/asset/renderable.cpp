#include <unordered_map>

#include "check.h"
#include "asset/texture.h"
#include "asset/renderable.h"

glsl_material create_lambertian(glm::vec3 const& albedo) {
    return glsl_material{
        glm::vec4{albedo, 1.0f},
        0.0f, 0.0f, -1, material_type::lambertian
    };
}

glsl_material create_lambertian(std::string_view texture_path,
    std::vector<struct texture_data>& texture_datas) {
    texture_datas.emplace_back(get_texture_data(texture_path));
    return glsl_material{{}, 0.0f, 0.0f, (int) texture_datas.size() - 1,
        material_type::lambertian};
}

glsl_material create_metal(glm::vec3 const& albedo, float fuzz) {
    return glsl_material{
        glm::vec4{albedo, 1.0f},
        fuzz, 0.0f, -1, material_type::metal
    };
}

glsl_material create_dielectric(float refraction_index) {
    return glsl_material{
        {}, 0.0f, refraction_index, -1, material_type::dielectric};
}

glsl_material create_diffuse_light(glm::vec3 const& albedo) {
    return glsl_material{
        glm::vec4{albedo, 1.0f},
        0.0f, 0.0f, -1, material_type::diffuse_light
    };
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
        .position = glm::vec4{upper_left, 1.0f},
        .normal = glm::vec4{normal, 0.0f},
        .tangent = {},
        .albedo_uv = glm::vec2{0.0f, 0.0f},
        .normal_uv = {},
    };
    glsl_triangle_vertex const v1{
        .position = glm::vec4{upper_left + u, 1.0f},
        .normal = glm::vec4{normal, 0.0f},
        .tangent = {},
        .albedo_uv = glm::vec2{1.0f, 0.0f},
        .normal_uv = {},
    };
    glsl_triangle_vertex const v2{
        .position = glm::vec4{upper_left + v, 1.0f},
        .normal = glm::vec4{normal, 0.0f},
        .tangent = {},
        .albedo_uv = glm::vec2{0.0f, 1.0f},
        .normal_uv = {},
    };
    glsl_triangle_vertex const v3{
        .position = glm::vec4{upper_left + u + v, 1.0f},
        .normal = glm::vec4{normal, 0.0f},
        .tangent = {},
        .albedo_uv = glm::vec2{1.0f, 1.0f},
        .normal_uv = {},
    };
    uint32_t const vertex_base_index = (uint32_t) mesh.vertices.size();
    uint32_t const material_index = (uint32_t) mesh.materials.size();
    glsl_triangle const t0{
        .a = vertex_base_index + 0,
        .b = vertex_base_index + 1,
        .c = vertex_base_index + 2,
        .material = material_index,
    };
    glsl_triangle const t1{
        .a = vertex_base_index + 2,
        .b = vertex_base_index + 1,
        .c = vertex_base_index + 3,
        .material = material_index,
    };
    mesh.vertices.push_back(v0);
    mesh.vertices.push_back(v1);
    mesh.vertices.push_back(v2);
    mesh.vertices.push_back(v3);
    mesh.triangles.push_back(t0);
    mesh.triangles.push_back(t1);
    mesh.materials.push_back(material);
}

void triangulate_sphere(
    triangle_mesh& mesh, glsl_sphere const& sphere, uint32_t segment_1d) {
    uint32_t const vertex_count_before = (uint32_t) mesh.vertices.size();
    uint32_t const material_count_before = (uint32_t) mesh.materials.size();
    float const sectorStep = 2 * glm::pi<float>() / (float) segment_1d;
    float const stackStep = glm::pi<float>() / (float) segment_1d;
    for (uint32_t i = 0; i <= segment_1d; ++i) {
        float const stackAngle =
            glm::pi<float>() / 2 -
            (float) i * stackStep;              // starting from pi/2 to -pi/2
        float const xy = std::cos(stackAngle);  // r * cos(u)
        float const nz = std::sin(stackAngle);
        float const z = nz * sphere.radius;  // r * sin(u)
        // add (sectorCount+1) vertices per stack
        // first and last vertices have same position and normal, but different
        // tex coords
        for (uint32_t j = 0; j <= segment_1d; ++j) {
            float const sectorAngle =
                (float) j * sectorStep;  // starting from 0 to 2pi
            float const x = sphere.radius * xy *
                            std::cos(sectorAngle);  // r * cos(u) * cos(v)
            float const y = sphere.radius * xy *
                            std::sin(sectorAngle);  // r * cos(u) * sin(v)
            float const nx = x;
            float const ny = y;
            float const s = (float) j / (float) segment_1d;
            float const t = (float) i / (float) segment_1d;
            mesh.vertices.push_back(glsl_triangle_vertex{
                .position = glm::vec4{glm::vec3{x, y, z} + sphere.radius, 1.0f},
                .normal = glm::vec4{nx, ny, nz, 0.0f},
                .tangent = {},
                .albedo_uv = glm::vec2{s, t},
                .normal_uv = {},
            });
        }
    }
    // generate CCW index list of sphere triangles
    // k1--k1+1
    // |  / |
    // | /  |
    // k2--k2+1
    for (uint32_t i = 0; i < segment_1d; ++i) {
        uint32_t k1 = i * (segment_1d + 1);
        uint32_t k2 = k1 + segment_1d + 1;
        for (uint32_t j = 0; j < segment_1d; ++j, ++k1, ++k2) {
            // k1 -> k2 -> k1 + 1
            if (i != 0) {
                glsl_triangle const t1{
                    .a = vertex_count_before + k1,
                    .b = vertex_count_before + k2,
                    .c = vertex_count_before + k1 + 1,
                    .material = material_count_before,
                };
                mesh.triangles.push_back(t1);
            }
            // k1 + 1 -> k2 -> k2 + 1
            if (i != segment_1d - 1) {
                glsl_triangle const t2{
                    .a = vertex_count_before + k1 + 1,
                    .b = vertex_count_before + k2,
                    .c = vertex_count_before + k2 + 1,
                    .material = material_count_before,
                };
                mesh.triangles.push_back(t2);
            }
        }
    }
    mesh.materials.push_back(sphere.material);
}
