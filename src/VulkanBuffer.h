#pragma once

#include "VulkanHeader.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

vk::Buffer create_storage_buffer(
    vk::Device dev, VmaAllocator alloc, vk::DeviceSize size) noexcept;
