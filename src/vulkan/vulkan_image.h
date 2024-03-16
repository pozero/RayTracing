#pragma once

#include "vulkan/vulkan_header.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

struct vma_image {
    vk::Image image{};
    VmaAllocation allocation = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    vk::Format format = vk::Format::eR8Unorm;
    void* mapped = nullptr;
    vk::ImageView primary_view{};
};

vma_image create_image(VmaAllocator vma_alloc, uint32_t width, uint32_t height,
    vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageCreateFlags flags, vk::ImageType type, vk::ImageUsageFlags usage,
    uint32_t levels, uint32_t layers, vk::ImageTiling tiling,
    vk::ImageLayout layout, VmaAllocationCreateFlags alloc_flags);

vma_image create_texture2d(vk::Device device, VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    uint32_t level, vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageUsageFlags usage);

vma_image create_cubemap(vk::Device device, VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    uint32_t level, vk::Format format, std::vector<uint32_t> const& queues);

vma_image create_host_image(VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    vk::Format format);

vk::Sampler create_default_sampler(vk::Device device);

void update_host_image(VmaAllocator vma_alloc, vma_image const& image,
    struct texture_data const& texture_data);

void update_texture2d(VmaAllocator vma_alloc, vma_image const& image,
    vk::CommandBuffer command_buffer, struct texture_data const& texture_data);

void destroy_image(
    vk::Device device, VmaAllocator vma_alloc, vma_image const& image);

void cleanup_staging_image(VmaAllocator vma_alloc);
