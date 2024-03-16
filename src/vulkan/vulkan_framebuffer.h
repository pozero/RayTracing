#pragma once

#include "vulkan/vulkan_header.h"

std::vector<vk::Framebuffer> create_framebuffers(vk::Device device,
    vk::RenderPass render_pass,
    std::vector<vk::ImageView> const& swapchain_image_views);

std::vector<vk::Framebuffer> create_framebuffers_with_depth_multisampling(
    vk::Device device, vk::RenderPass render_pass,
    std::vector<vk::ImageView> const& swapchain_image_views,
    vk::ImageView depth_view, vk::ImageView color_view);
