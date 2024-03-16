#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

#include "vulkan/vulkan_header.h"

#define VK_DBG 1

void create_render_context();

void destroy_render_context();

vk::CommandBuffer get_command_buffer(vk::PipelineBindPoint bind_point);

void submit_command_buffer(vk::PipelineBindPoint bind_point,
    std::vector<vk::Semaphore> const& wait_semphores,
    std::vector<vk::PipelineStageFlags> const& wait_stages,
    std::vector<vk::Semaphore> const& signal_semphores);

uint32_t constexpr FRAME_IN_FLIGHT = 3;

extern struct GLFWwindow* window;
extern vk::SurfaceKHR surface;
extern vk::Device device;
extern vk::PhysicalDevice physical_device;
extern struct vulkan_queues command_queues;
extern VmaAllocator vma_alloc;
