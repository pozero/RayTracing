#include <vector>

#include "check.h"
#include "vulkan/vulkan_framebuffer.h"
#include "vulkan/vulkan_swapchain.h"

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

std::vector<vk::Framebuffer> create_framebuffers_with_depth_multisampling(
    vk::Device device, vk::RenderPass render_pass,
    std::vector<vk::ImageView> const& swapchain_image_views,
    vk::ImageView depth_view, vk::ImageView color_view) {
    vk::Result result;
    std::vector<vk::Framebuffer> framebuffers(swapchain_image_views.size());
    for (uint32_t i = 0; i < swapchain_image_views.size(); ++i) {
        std::array const attachments{
            color_view, depth_view, swapchain_image_views[i]};
        vk::FramebufferCreateInfo const framebuffer_info{
            .renderPass = render_pass,
            .attachmentCount = (uint32_t) attachments.size(),
            .pAttachments = attachments.data(),
            .width = swapchain_extent.width,
            .height = swapchain_extent.height,
            .layers = 1,
        };
        VK_CHECK_CREATE(result, framebuffers[i],
            device.createFramebuffer(framebuffer_info));
    }
    return framebuffers;
}
