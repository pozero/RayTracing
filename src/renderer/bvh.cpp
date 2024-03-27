#include "check.h"
#include "bvh.h"
#include "utils/to_span.h"

std::tuple<std::vector<bvh_linear_node>, std::vector<glsl_instance>> create_bvh(
    scene& scene) {
    std::vector<uint32_t> mesh_vertex_count{};
    std::vector<aabb> mesh_aabbs{};
    std::vector<glsl_instance> instances{};
    std::vector<aabb> instance_aabbs{};
    std::vector<aabb> triangle_aabbs{};
    mesh_vertex_count.reserve(scene.mesh_vertex_start.size());
    mesh_aabbs.reserve(scene.mesh_vertex_start.size());
    instances.reserve(scene.primitives.size() + scene.lights.size());
    instance_aabbs.reserve(scene.primitives.size() + scene.lights.size());
    for (uint32_t s = 1; s < scene.mesh_vertex_start.size(); ++s) {
        mesh_vertex_count.push_back(
            scene.mesh_vertex_start[s] - scene.mesh_vertex_start[s - 1]);
    }
    mesh_vertex_count.push_back(
        (uint32_t) scene.vertices.size() -
        (mesh_vertex_count.empty() ? 0 : mesh_vertex_count.back()));
    for (uint32_t m = 0; m < scene.mesh_vertex_start.size(); ++m) {
        mesh_aabbs.push_back(create_aabb(to_span(
            scene.vertices, scene.mesh_vertex_start[m], mesh_vertex_count[m])));
    }
    for (uint32_t p = 0; p < scene.primitives.size(); ++p) {
        primitive const& prim = scene.primitives[p];
        glsl_instance const inst{
            .mesh = prim.mesh,
            .transform = prim.transform,
            .material = prim.material,
            .medium = prim.medium,
            .light = -1,
        };
        instances.push_back(inst);
        aabb const aabb =
            create_aabb(to_span(scene.vertices,
                            scene.mesh_vertex_start[(uint32_t) prim.mesh],
                            mesh_vertex_count[(uint32_t) prim.mesh]),
                scene.transformation[(uint32_t) prim.transform]);
        instance_aabbs.push_back(aabb);
    }
    for (uint32_t l = 0; l < scene.lights.size(); ++l) {
        light const& light = scene.lights[l];
        if (light.type == light_type::distant) {
            continue;
        }
        glsl_instance const inst{
            .mesh = light.mesh,
            .transform = light.transform,
            .material = -1,
            .medium = -1,
            .light = (int32_t) l,
        };
        instances.push_back(inst);
        aabb const aabb =
            create_aabb(to_span(scene.vertices,
                            scene.mesh_vertex_start[(uint32_t) light.mesh],
                            mesh_vertex_count[(uint32_t) light.mesh]),
                scene.transformation[(uint32_t) light.transform]);
        instance_aabbs.push_back(aabb);
    }
    CHECK(scene.vertices.size() % 3 == 0, "");
    for (uint32_t t = 0; t < scene.vertices.size() / 3; ++t) {
        triangle_aabbs.push_back(
            create_aabb(to_span(scene.vertices, 3 * t, 3)));
    }
}
