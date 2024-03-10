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

void cook_torrance_brdf_renderer();
