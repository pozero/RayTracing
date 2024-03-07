#pragma once

#include "vulkan_header.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

struct vma_buffer {
    vk::Buffer buffer;
    VmaAllocation allocation;
    uint8_t* mapped;
    uint32_t size;
};

vma_buffer create_buffer(VmaAllocator vma_alloc, uint32_t size,
    std::vector<uint32_t> const& queues, VkBufferCreateFlags buffer_flags,
    VkBufferUsageFlags usage_flag, VmaAllocationCreateFlags alloc_flags);

void destory_buffer(VmaAllocator vma_alloc, vma_buffer const& buffer);

vma_buffer create_gpu_only_buffer(VmaAllocator vma_alloc, uint32_t size,
    std::vector<uint32_t> const& queues, vk::BufferUsageFlags usage);

vma_buffer create_staging_buffer(
    VmaAllocator vma_alloc, uint32_t size, std::vector<uint32_t> const& queues);

vma_buffer create_frequent_readwrite_buffer(VmaAllocator vma_alloc,
    uint32_t size, std::vector<uint32_t> const& queues,
    vk::BufferUsageFlags usage);

void update_buffer(VmaAllocator vma_alloc, vk::CommandBuffer command_buffer,
    vma_buffer const& buffer, std::span<const uint8_t> data, uint32_t offset);

void cleanup_staging_buffer(VmaAllocator vma_alloc);

void create_dummy_buffer(VmaAllocator vma_alloc);

void destroy_dummy_buffer(VmaAllocator vma_alloc);
