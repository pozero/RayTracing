#pragma once

#include "vulkan/vulkan_header.h"

vk::Pipeline create_graphics_pipeline(vk::Device device,
    std::string_view vert_path, std::string_view frag_path,
    vk::PipelineLayout layout, vk::RenderPass render_pass);

vk::Pipeline create_compute_pipeline(vk::Device device,
    std::string_view comp_path, vk::PipelineLayout layout,
    std::vector<uint32_t> const& specialization_constants);
