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
    return 0;
}
