#pragma once

#include <utility>
#include <vector>

#include "vulkan/vulkan_device.h"
#include "vulkan/vulkan_header.h"

extern vk::Extent2D swapchain_extent;
extern vk::SurfaceFormatKHR surface_format;
extern vk::SampleCountFlagBits multisample_count;

void prepare_swapchain(vk::PhysicalDevice physical_dev, vk::SurfaceKHR surface);

std::tuple<vk::SwapchainKHR, std::vector<vk::Image>, std::vector<vk::ImageView>>
    create_swapchain(
        vk::Device device, vk::SurfaceKHR surface, vulkan_queues const& queues);

std::tuple<vk::SwapchainKHR, std::vector<vk::ImageView>, struct vk_image,
    struct vk_image>
    create_swapchain_with_depth_multisampling(vk::Device device,
        VmaAllocator vma_alloc, vk::SurfaceKHR surface,
        vulkan_queues const& queues);

void wait_window(vk::Device device, vk::PhysicalDevice physical_dev,
    vk::SurfaceKHR surface, struct GLFWwindow* window);

// vk::Result::eErrorOutOfDateKHR was considered an error in vulkan hpp, so just
// warp that error here
vk::Result swapchain_acquire_next_image_wrapper(vk::Device device,
    vk::SwapchainKHR swapchain, uint64_t timeout, vk::Semaphore semaphore,
    vk::Fence fence, uint32_t* image_idx);

vk::Result swapchain_present_wrapper(
    vk::Queue queue, vk::PresentInfoKHR const& present_info);
