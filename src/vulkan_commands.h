#pragma once

#include "vulkan_header.h"

std::pair<vk::CommandPool, std::vector<vk::CommandBuffer>>
    create_command_buffer(
        vk::Device device, uint32_t queue_idx, size_t command_buffer_count);
