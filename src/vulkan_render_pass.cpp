#include "vulkan_render_pass.h"
#include "vulkan_check.h"
#include "vulkan_swapchain.h"

vk::RenderPass create_render_pass(vk::Device device) {
    vk::Result result;
    vk::RenderPass render_pass;
    vk::AttachmentDescription const color_attachment_desc{
        .format = surface_format.format,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference const color_attachment_reference{
        .attachment = 0,
        .layout = vk::ImageLayout::eAttachmentOptimal,
    };
    vk::SubpassDescription const subpass_desc{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference,
    };
    vk::RenderPassCreateInfo const render_pass_info{
        .attachmentCount = 1,
        .pAttachments = &color_attachment_desc,
        .subpassCount = 1,
        .pSubpasses = &subpass_desc,
        .dependencyCount = 0,
    };
    VK_CHECK_CREATE(
        result, render_pass, device.createRenderPass(render_pass_info));
    return render_pass;
}
