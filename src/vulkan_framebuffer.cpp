#include <vector>

#include "vulkan_framebuffer.h"
#include "vulkan_check.h"
#include "vulkan_swapchain.h"

std::vector<vk::Framebuffer> create_framebuffers(vk::Device device,
    vk::RenderPass render_pass,
    std::vector<vk::ImageView> const& swapchain_image_views) {
    vk::Result result;
    std::vector<vk::Framebuffer> framebuffers(swapchain_image_views.size());
    for (uint32_t i = 0; i < swapchain_image_views.size(); ++i) {
        vk::FramebufferCreateInfo const framebuffer_info{
            .renderPass = render_pass,
            .attachmentCount = 1,
            .pAttachments = &swapchain_image_views[i],
            .width = swapchain_extent.width,
            .height = swapchain_extent.height,
            .layers = 1,
        };
        VK_CHECK_CREATE(result, framebuffers[i],
            device.createFramebuffer(framebuffer_info));
    }
    return framebuffers;
}
