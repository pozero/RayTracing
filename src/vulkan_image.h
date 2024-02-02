#pragma once

#include "vulkan_header.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

struct vma_image {
    vk::Image image;
    VmaAllocation allocation;
};
