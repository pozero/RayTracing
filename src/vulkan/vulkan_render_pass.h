#pragma once

#include "vulkan/vulkan_header.h"

vk::RenderPass create_render_pass(vk::Device device);

vk::RenderPass create_render_pass_with_depth(vk::Device device);

vk::RenderPass create_render_pass_with_depth_multisampling(vk::Device device);
