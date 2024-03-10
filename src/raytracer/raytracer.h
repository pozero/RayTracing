#pragma once

#include "check.h"

#include "camera.h"
#include "window.h"

#include "vulkan/vulkan_device.h"
#include "vulkan/vulkan_swapchain.h"
#include "vulkan/vulkan_render_pass.h"
#include "vulkan/vulkan_framebuffer.h"
#include "vulkan/vulkan_commands.h"
#include "vulkan/vulkan_descriptor.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_buffer.h"
#include "vulkan/vulkan_image.h"

#include "utils/file.h"
#include "utils/to_span.h"
#include "utils/high_resolution_clock.h"

#include "asset/renderable.h"
#include "asset/texture.h"

#define VK_DBG 1

std::pair<uint32_t, uint32_t> get_frame_size1();
std::tuple<camera, std::vector<glsl_sphere>, triangle_mesh,
    std::vector<texture_data>>
    get_scene1(uint32_t frame_width, uint32_t frame_height);
glsl_sky_color get_sky_color1();

std::pair<uint32_t, uint32_t> get_frame_size2();
std::tuple<camera, std::vector<glsl_sphere>, triangle_mesh,
    std::vector<texture_data>>
    get_scene2(uint32_t frame_width, uint32_t frame_height);
glsl_sky_color get_sky_color2();

std::pair<uint32_t, uint32_t> get_frame_size3();
std::tuple<camera, std::vector<glsl_sphere>, triangle_mesh,
    std::vector<texture_data>>
    get_scene3(uint32_t frame_width, uint32_t frame_height);
glsl_sky_color get_sky_color3();

void brute_force_raytracer();
