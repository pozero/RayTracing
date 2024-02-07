#pragma once

#include "vulkan_header.h"

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
};

vma_image create_image(VmaAllocator vma_alloc, uint32_t width, uint32_t height,
    vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageCreateFlags flags, vk::ImageType type, vk::ImageUsageFlags usage,
    uint32_t levels, uint32_t layers, vk::ImageTiling tiling,
    vk::ImageLayout layout, VmaAllocationCreateFlags alloc_flags);

std::pair<vma_image, vk::ImageView> create_texture2d_simple(vk::Device device,
    VmaAllocator vma_alloc, vk::CommandBuffer command_buffer, uint32_t width,
    uint32_t height, vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageUsageFlags usage);

vk::Sampler create_default_sampler(vk::Device device);

void destroy_image(VmaAllocator vma_alloc, vma_image const& image);
