#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

#include "vulkan/vulkan_header.h"

// #define VK_DBG

bool window_should_close();

void poll_window_event();

void create_render_context();

void destroy_render_context();

void wait_vulkan();

std::pair<vk::CommandBuffer, uint32_t> get_command_buffer(
    vk::PipelineBindPoint bind_point);

void add_submit_wait(vk::PipelineBindPoint bind_point, vk::Semaphore semaphore,
    vk::PipelineStageFlags stage);

void add_submit_signal(
    vk::PipelineBindPoint bind_point, vk::Semaphore semaphore);

void submit_command_buffer(vk::PipelineBindPoint bind_point);

void add_present_wait(vk::Semaphore semaphore);

vk::Result present(
    vk::SwapchainKHR const& swapchain, uint32_t const& image_idx);

uint32_t constexpr FRAME_IN_FLIGHT = 3;

extern uint32_t win_width;
extern uint32_t win_height;
extern struct GLFWwindow* window;
extern vk::SurfaceKHR surface;
extern vk::Device device;
extern vk::PhysicalDevice physical_device;
extern struct vulkan_queues command_queues;
extern VmaAllocator vma_alloc;
