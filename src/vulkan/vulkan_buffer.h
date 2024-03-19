#pragma once

#include "vulkan/vulkan_header.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

struct vk_buffer {
    vk::Buffer buffer{};
    VmaAllocation allocation = nullptr;
    uint8_t* mapped = nullptr;
    uint32_t size = 0;
};

vk_buffer create_buffer(VmaAllocator vma_alloc, uint32_t size,
    std::vector<uint32_t> const& queues, VkBufferCreateFlags buffer_flags,
    VkBufferUsageFlags usage_flag, VmaAllocationCreateFlags alloc_flags);

void destroy_buffer(VmaAllocator vma_alloc, vk_buffer const& buffer);

vk_buffer create_gpu_only_buffer(VmaAllocator vma_alloc, uint32_t size,
    std::vector<uint32_t> const& queues, vk::BufferUsageFlags usage);

vk_buffer create_staging_buffer(
    VmaAllocator vma_alloc, uint32_t size, std::vector<uint32_t> const& queues);

vk_buffer create_frequent_readwrite_buffer(VmaAllocator vma_alloc,
    uint32_t size, std::vector<uint32_t> const& queues,
    vk::BufferUsageFlags usage);

void update_buffer(VmaAllocator vma_alloc, vk::CommandBuffer command_buffer,
    vk_buffer const& buffer, std::span<const uint8_t> data, uint32_t offset);

void cleanup_staging_buffer(VmaAllocator vma_alloc);

void create_dummy_buffer(VmaAllocator vma_alloc);

void destroy_dummy_buffer(VmaAllocator vma_alloc);
