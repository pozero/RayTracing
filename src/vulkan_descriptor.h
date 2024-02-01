#pragma once

#include "vulkan_header.h"

struct vk_descriptor_set_binding {
    vk::DescriptorType descriptor_type;
    uint32_t array_count;
};

vk::DescriptorSetLayout create_descritpro_set_layout(vk::Device device,
    vk::ShaderStageFlagBits shader_stage,
    std::vector<vk_descriptor_set_binding> const& bindings);

vk::PipelineLayout create_pipeline_layout(vk::Device device,
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layouts);

vk::PipelineLayout create_pipeline_layout(vk::Device device,
    uint32_t push_constant_size, vk::ShaderStageFlagBits push_constant_stage,
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layouts);
