#pragma once

#include "vulkan_header.h"

vk::Pipeline create_graphics_pipeline(vk::Device device,
    std::string_view vert_path, std::string_view frag_path,
    vk::PipelineLayout layout, vk::RenderPass render_pass);
