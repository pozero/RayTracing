#include "raytracer/raytracer.h"
#include "renderer/renderer.h"

#define BRUTE_FORCE_RAYTRACER 0
#define COOK_TORRANCE_BRDF_RENDERER 1

int main() {
    switch (COOK_TORRANCE_BRDF_RENDERER) {
        case BRUTE_FORCE_RAYTRACER:
            brute_force_raytracer();
            break;
        case COOK_TORRANCE_BRDF_RENDERER:
            cook_torrance_brdf_renderer();
            break;
        default:
            break;
    }

    // sphere triangulate test
    // triangle_mesh mesh{};
    // glsl_sphere sphere = create_sphere(glm::vec3{0.0f, 0.0f, 0.0f}, 2.0f,
    // {}); triangulate_sphere(mesh, sphere, 4); for (uint32_t i = 0; i <
    // mesh.vertices.size(); ++i) {
    //     glsl_triangle_vertex const& v = mesh.vertices[i];
    //     fmt::println(
    //         "[{}] position({:.1f}, {:.1f}, {:.1f}, {:.1f}) normal({:.1f}, "
    //         "{:.1f}, {:.1f}, {:.1f}) uv({:.1f}, {:.1f})",
    //         i, v.position.x, v.position.y, v.position.z, v.position.w,
    //         v.normal.x, v.normal.y, v.normal.z, v.normal.w, v.albedo_uv.x,
    //         v.albedo_uv.y);
    // }
    // for (uint32_t i = 0; i < mesh.triangles.size(); ++i) {
    //     glsl_triangle const& t = mesh.triangles[i];
    //     fmt::println("[{}] a: {} b: {} c: {} material: {}", i, t.a, t.b, t.c,
    //         t.material);
    // }

    return 0;
}
