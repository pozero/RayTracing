#pragma once

#include "vulkan/vulkan_header.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

#include <span>

struct vulkan_queues {
    vk::Queue graphics_queue;
    vk::Queue present_queue;
    vk::Queue compute_queue;
    uint32_t graphics_queue_idx;
    uint32_t present_queue_idx;
    uint32_t compute_queue_idx;
};

vk::DynamicLoader load_vulkan() noexcept;

vk::Instance create_instance(std::span<const char*> inst_ext,
    std::span<const char*> inst_layer) noexcept;

vk::DebugUtilsMessengerEXT create_debug_messenger(vk::Instance inst) noexcept;

vk::SurfaceKHR create_surface(
    vk::Instance inst, struct GLFWwindow* window) noexcept;

std::tuple<vk::Device, vk::PhysicalDevice, vulkan_queues>
    select_physical_device_create_device_queues(vk::Instance inst,
        vk::SurfaceKHR surface, std::span<const char*> dev_ext,
        const void* dev_creation_pnext,
        vk::PhysicalDeviceFeatures const& enabled_features) noexcept;

VmaAllocator create_vma_allocator(
    vk::Instance inst, vk::PhysicalDevice phy_dev, vk::Device dev) noexcept;
