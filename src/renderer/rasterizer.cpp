#include "vulkan/vulkan_swapchain.h"
#include "vulkan/vulkan_render_pass.h"
#include "vulkan/vulkan_framebuffer.h"
#include "vulkan/vulkan_descriptor.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_buffer.h"
#include "vulkan/vulkan_image.h"

#include "asset/camera.h"
#include "asset/scene.h"
#include "asset/texture.h"

#include "renderer/renderer.h"

void rasterizer_initialize();
void rasterizer_prepare_data(scene const& scene);
void rasterizer_update_data(camera const& camera, scene const& scene);
void rasterizer_render();
void rasterizer_destroy();

void load_rasterizer(renderer& renderer) {
    renderer.initialize = rasterizer_initialize;
    renderer.prepare_data = rasterizer_prepare_data;
    renderer.update_data = rasterizer_update_data;
    renderer.render = rasterizer_render;
    renderer.destroy = rasterizer_destroy;
}

void rasterizer_initialize() {
}

void rasterizer_prepare_data(scene const& scene) {
}

void rasterizer_update_data(camera const& camera, scene const& scene) {
}

void rasterizer_render() {
}

void rasterizer_destroy() {
}
