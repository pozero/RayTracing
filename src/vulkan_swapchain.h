#pragma once

#include <utility>
#include <vector>

#include "vulkan_header.h"

extern vk::Extent2D swapchain_extent;
extern vk::SurfaceFormatKHR surface_format;

void prepare_swapchain(vk::PhysicalDevice physical_dev, vk::SurfaceKHR surface);

std::tuple<vk::SwapchainKHR, std::vector<vk::Image>, std::vector<vk::ImageView>>
    create_swapchain(vk::Device device, vk::PhysicalDevice physical_dev,
        vk::SurfaceKHR surface, struct GLFWwindow* window,
        struct vulkan_queues const& queues,
        vk::SwapchainKHR old_swapchain = {});
