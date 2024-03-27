#include "check.h"
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
#include "renderer/render_context.h"

#include "utils/to_span.h"
#include "utils/file.h"

#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"

void raytracer_initialize();
void raytracer_prepare_data(scene const& scene);
void raytracer_update_data(scene const& scene);
void raytracer_render(camera const& camera);
void raytracer_present();
void raytracer_destroy();
