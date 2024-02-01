#pragma once

#include "vulkan_header.h"

std::vector<vk::Framebuffer> create_framebuffers(vk::Device device,
    vk::RenderPass render_pass,
    std::vector<vk::ImageView> const& swapchain_image_views);
