#pragma once

#include <vector>

#include "vulkan/vulkan_header.h"

vk::ShaderModule create_shader_module(
    vk::Device device, std::string_view file_path);

vk::SpecializationInfo create_specialization_info(
    std::vector<uint32_t> const &constants,
    std::vector<vk::SpecializationMapEntry> &entries);
