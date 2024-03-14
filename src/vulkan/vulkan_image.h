#pragma once

#include "vulkan/vulkan_header.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

struct vma_image {
    vk::Image image;
    VmaAllocation allocation;
    uint32_t width;
    uint32_t height;
    vk::Format format;
    void* mapped;
    vk::ImageView primary_view;
    std::vector<vk::ImageView> mipmap_views;
};

vma_image create_image(VmaAllocator vma_alloc, uint32_t width, uint32_t height,
    vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageCreateFlags flags, vk::ImageType type, vk::ImageUsageFlags usage,
    uint32_t levels, uint32_t layers, vk::ImageTiling tiling,
    vk::ImageLayout layout, VmaAllocationCreateFlags alloc_flags);

vma_image create_texture2d_simple(vk::Device device, VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageUsageFlags usage);

vma_image create_cubemap(vk::Device device, VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    vk::Format format, std::vector<uint32_t> const& queues);

vma_image create_cubemap_with_mipmap(vk::Device device, VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    uint32_t level, vk::Format format, std::vector<uint32_t> const& queues);

vma_image create_host_image(VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    vk::Format format);

vk::Sampler create_default_sampler(vk::Device device);

void update_host_image(VmaAllocator vma_alloc, vma_image const& image,
    struct texture_data const& texture_data);

void update_texture2d_simple(VmaAllocator vma_alloc, vma_image const& image,
    vk::CommandBuffer command_buffer, struct texture_data const& texture_data);

void destroy_image(
    vk::Device device, VmaAllocator vma_alloc, vma_image const& image);

void cleanup_staging_image(VmaAllocator vma_alloc);
