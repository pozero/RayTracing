#pragma once

#include <array>

#include "vulkan_header.h"

vk::ShaderModule create_shader_module(
    vk::Device device, std::string_view file_path);

// GL_ARB_gpu_shader_int64
template <size_t N>
vk::SpecializationInfo create_specialization_info(
    std::array<uint64_t, N> const &constants,
    std::array<vk::SpecializationMapEntry, N> &entries) {
    for (size_t i = 0; i < N; ++i) {
        entries[i].constantID = i;
        entries[i].offset = i * sizeof(uint64_t);
        entries[i].size = sizeof(uint64_t);
    }
    vk::SpecializationInfo specialization_info{
        .mapEntryCount = N,
        .pMapEntries = entries.data(),
        .dataSize = (uint32_t) constants.size() * sizeof(uint64_t),
        .pData = constants.data(),
    };
    return specialization_info;
}
