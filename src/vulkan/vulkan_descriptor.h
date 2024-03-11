#pragma once

#include "vulkan/vulkan_header.h"

struct vk_descriptor_set_binding {
    vk::DescriptorType descriptor_type;
    uint32_t array_count;
    vk::DescriptorBindingFlags flags{};
};

vk::DescriptorSetLayout create_descriptor_set_layout(vk::Device device,
    vk::ShaderStageFlagBits shader_stage,
    std::vector<vk_descriptor_set_binding> const& bindings,
    vk::DescriptorSetLayoutCreateFlags flags = {});

vk::PipelineLayout create_pipeline_layout(vk::Device device,
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layouts);

vk::PipelineLayout create_pipeline_layout(vk::Device device,
    std::vector<uint32_t> const& push_constant_size,
    std::vector<vk::ShaderStageFlagBits> const& push_constant_stage,
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layouts);

vk::DescriptorPool create_descriptor_pool(
    vk::Device device, vk::DescriptorPoolCreateFlags flags = {});

std::vector<vk::DescriptorSet> create_descriptor_set(vk::Device device,
    vk::DescriptorPool descriptor_pool,
    vk::DescriptorSetLayout descriptor_set_layout, uint32_t count,
    uint32_t variable_size = 0);

void update_descriptor(vk::Device device, vk::DescriptorSet descriptor_set,
    uint32_t binding, uint32_t array_idx, vk::DescriptorType type,
    vk::DescriptorImageInfo const* image_info,
    vk::DescriptorBufferInfo const* buffer_info);

void update_descriptor_image_sampler_combined(vk::Device device,
    vk::DescriptorSet descriptor_set, uint32_t binding, uint32_t array_idx,
    vk::Sampler sampler, vk::ImageView view);

void update_descriptor_storage_image(vk::Device device,
    vk::DescriptorSet descriptor_set, uint32_t binding, uint32_t array_idx,
    vk::ImageView view);

void update_descriptor_storage_buffer_whole(vk::Device device,
    vk::DescriptorSet descriptor_set, uint32_t binding, uint32_t array_idx,
    struct vma_buffer const& buffer);

void update_descriptor_uniform_buffer_whole(vk::Device device,
    vk::DescriptorSet descriptor_set, uint32_t binding, uint32_t array_idx,
    struct vma_buffer const& buffer);
