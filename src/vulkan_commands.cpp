#include <vector>
#include <utility>

#include "vulkan_commands.h"
#include "vulkan_check.h"

std::pair<vk::CommandPool, std::vector<vk::CommandBuffer>>
    create_command_buffer(
        vk::Device device, uint32_t queue_idx, size_t command_buffer_count) {
    vk::Result result;
    vk::CommandPool command_pool;
    std::vector<vk::CommandBuffer> command_buffers(command_buffer_count);
    vk::CommandPoolCreateInfo const command_pool_info{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queue_idx,
    };
    VK_CHECK_CREATE(
        result, command_pool, device.createCommandPool(command_pool_info));
    vk::CommandBufferAllocateInfo const command_buffer_allocation_info{
        .commandPool = command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = (uint32_t) command_buffer_count,
    };
    VK_CHECK_CREATE(result, command_buffers,
        device.allocateCommandBuffers(command_buffer_allocation_info));
    return std::make_pair(command_pool, command_buffers);
}
