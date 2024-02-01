#include <vector>
#include <iostream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "fmt/core.h"
#include "GLFW/glfw3.h"
#pragma clang diagnostic pop

#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_render_pass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_commands.h"
#include "vulkan_descriptor.h"
#include "vulkan_pipeline.h"
#include "vulkan_check.h"
#include "file.h"

#define VK_DBG 1

GLFWwindow* glfw_create_window(int width, int height);

#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
int main() {
    //////////////////////////////
    ///* Initialization: GLFW *///
    //////////////////////////////
    int constexpr win_width = 800;
    int constexpr win_height = 600;
    GLFWwindow* window = glfw_create_window(win_width, win_height);
    //////////////////////////////
    ///* Initialization: GLFW *///
    //////////////////////////////

    vk::Result result;

    //////////////////////////////////
    ///* Initialization: Instance *///
    //////////////////////////////////
    std::vector<const char*> inst_ext{};
    std::vector<const char*> inst_layer{};
    std::vector<const char*> dev_ext{};
    {
        uint32_t glfw_required_inst_ext_cnt = 0;
        auto const glfw_required_inst_ext_name =
            glfwGetRequiredInstanceExtensions(&glfw_required_inst_ext_cnt);
        std::copy(glfw_required_inst_ext_name,
            glfw_required_inst_ext_name + glfw_required_inst_ext_cnt,
            std::back_inserter(inst_ext));
    }
    dev_ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#if defined(VK_DBG)
    inst_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    inst_layer.push_back("VK_LAYER_KHRONOS_validation");
#endif
    const void* dev_creation_pnext = nullptr;
    vk::PhysicalDeviceSynchronization2Features const synchron2_feature{
        .synchronization2 = VK_TRUE};
    dev_creation_pnext = &synchron2_feature;
    vk::DynamicLoader vk_loader = load_vulkan();
    vk::Instance vk_inst = create_instance(inst_ext, inst_layer);
    //////////////////////////////////
    ///* Initialization: Instance *///
    //////////////////////////////////

#if defined(VK_DBG)
    vk::DebugUtilsMessengerEXT vk_dbg_messenger =
        create_debug_messenger(vk_inst);
#endif

    /////////////////////////////////
    ///* Initialization: Surface *///
    /////////////////////////////////
    vk::SurfaceKHR vk_surface = create_surface(vk_inst, window);
    /////////////////////////////////
    ///* Initialization: Surface *///
    /////////////////////////////////

    ////////////////////////////////
    ///* Initialization: Device *///
    ////////////////////////////////
    auto [vk_dev, vk_phy_dev, vk_queues] =
        select_physical_device_create_device_queues(
            vk_inst, vk_surface, dev_ext, dev_creation_pnext);
    fmt::println(
        "Selected device: {}", vk_phy_dev.getProperties().deviceName.data());
    ////////////////////////////////
    ///* Initialization: Device *///
    ////////////////////////////////

    /////////////////////////////
    ///* Initialization: VMA *///
    /////////////////////////////
    VmaAllocator vma_alloc = create_vma_allocator(vk_inst, vk_phy_dev, vk_dev);
    /////////////////////////////
    ///* Initialization: VMA *///
    /////////////////////////////

    ///////////////////////////////////
    ///* Initialization: Swapchain *///
    ///////////////////////////////////
    prepare_swapchain(vk_phy_dev, vk_surface);
    auto [vk_swapchain, vk_swapchain_images, vk_swapchain_image_views] =
        create_swapchain(vk_dev, vk_phy_dev, vk_surface, window, vk_queues);
    ///////////////////////////////////
    ///* Initialization: Swapchain *///
    ///////////////////////////////////

    /////////////////////////////////////////////////////
    ///* Initialization: Render Pass And Framebuffer *///
    /////////////////////////////////////////////////////
    vk::RenderPass vk_render_pass = create_render_pass(vk_dev);
    std::vector<vk::Framebuffer> vk_framebuffers =
        create_framebuffers(vk_dev, vk_render_pass, vk_swapchain_image_views);
    /////////////////////////////////////////////////////
    ///* Initialization: Render Pass And Framebuffer *///
    /////////////////////////////////////////////////////

    size_t constexpr FRAME_IN_FLIGHT = 3;

    /////////////////////////////////////////
    ///* Initialization: Command Buffers *///
    /////////////////////////////////////////
    auto [vk_graphics_command_pool, vk_graphics_command_buffers] =
        create_command_buffer(
            vk_dev, vk_queues.graphics_queue_idx, FRAME_IN_FLIGHT);
    /////////////////////////////////////////
    ///* Initialization: Command Buffers *///
    /////////////////////////////////////////

    /////////////////////////////////////
    ///* Initialization: Descriptors *///
    /////////////////////////////////////
    vk::PipelineLayout vk_graphics_pipeline_layout =
        create_pipeline_layout(vk_dev, {});
    /////////////////////////////////////
    ///* Initialization: Descriptors *///
    /////////////////////////////////////

    ///////////////////////////////////////////
    ///* Initialization: Graphics Pipeline *///
    ///////////////////////////////////////////
    vk::Pipeline vk_graphics_pipeline = create_graphics_pipeline(vk_dev,
        PATH_FROM_BINARY("shaders/rect.vert.spv"),
        PATH_FROM_BINARY("shaders/rect.frag.spv"), vk_graphics_pipeline_layout,
        vk_render_pass);
    ///////////////////////////////////////////
    ///* Initialization: Graphics Pipeline *///
    ///////////////////////////////////////////

    ///////////////////////////////////////////
    ///* Initialization: Frame Sync Object *///
    ///////////////////////////////////////////
    uint32_t frame_idx = 0;
    vk::FenceCreateInfo const fence_info{
        .flags = vk::FenceCreateFlagBits::eSignaled,
    };
    vk::SemaphoreCreateInfo const semaphore_info{};
    std::vector<vk::Fence> render_fences(FRAME_IN_FLIGHT);
    std::vector<vk::Semaphore> render_semaphores(FRAME_IN_FLIGHT);
    std::vector<vk::Semaphore> present_semaphores(FRAME_IN_FLIGHT);
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        VK_CHECK_CREATE(
            result, render_fences[i], vk_dev.createFence(fence_info));
        VK_CHECK_CREATE(result, render_semaphores[i],
            vk_dev.createSemaphore(semaphore_info));
        VK_CHECK_CREATE(result, present_semaphores[i],
            vk_dev.createSemaphore(semaphore_info));
    }
    ///////////////////////////////////////////
    ///* Initialization: Frame Sync Object *///
    ///////////////////////////////////////////

    ////////////////////
    ///* RenderLoop *///
    ////////////////////
    while (!glfwWindowShouldClose(window)) {
        uint32_t const frame_sync_idx = frame_idx % FRAME_IN_FLIGHT;
        vk::Fence const render_fence = render_fences[frame_sync_idx];
        vk::Semaphore const render_semaphore =
            render_semaphores[frame_sync_idx];
        vk::Semaphore const present_semaphore =
            present_semaphores[frame_sync_idx];
        vk::CommandBuffer const command_buffer =
            vk_graphics_command_buffers[frame_sync_idx];
        VK_CHECK(result, vk_dev.waitForFences(1, &render_fence, true, 1e9));
        uint32_t swapchain_image_idx = 0;
        result = vk_dev.acquireNextImageKHR(vk_swapchain, 1e9,
            present_semaphore, nullptr, &swapchain_image_idx);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            std::tie(vk_swapchain, vk_swapchain_images,
                vk_swapchain_image_views) = create_swapchain(vk_dev, vk_phy_dev,
                vk_surface, window, vk_queues);
            continue;
        }
        CHECK(result == vk::Result::eSuccess ||
                  result == vk::Result::eSuboptimalKHR,
            "");
        VK_CHECK(result, vk_dev.resetFences(1, &render_fence));
        VK_CHECK(result, command_buffer.reset());
        vk::CommandBufferBeginInfo const begin_info{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };
        VK_CHECK(result, command_buffer.begin(begin_info));
        vk::Rect2D const render_area{
            .offset = {0, 0},
            .extent = swapchain_extent,
        };
        vk::ClearValue const color_clear_val{
            .color = {std::array{0.243f, 0.706f, 0.537f, 1.0f}},
        };
        vk::RenderPassBeginInfo const render_pass_begin_info{
            .renderPass = vk_render_pass,
            .framebuffer = vk_framebuffers[swapchain_image_idx],
            .renderArea = render_area,
            .clearValueCount = 1,
            .pClearValues = &color_clear_val,
        };
        command_buffer.beginRenderPass(
            render_pass_begin_info, vk::SubpassContents::eInline);
        vk::Viewport const viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) swapchain_extent.width,
            .height = (float) swapchain_extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        command_buffer.setViewport(0, 1, &viewport);
        vk::Rect2D const scissor = render_area;
        command_buffer.setScissor(0, 1, &scissor);
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics, vk_graphics_pipeline);
        command_buffer.draw(3, 1, 0, 0);
        command_buffer.endRenderPass();
        VK_CHECK(result, command_buffer.end());
        vk::PipelineStageFlags const wait_stage =
            vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo const submit_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &present_semaphore,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_semaphore,
        };
        VK_CHECK(result,
            vk_queues.graphics_queue.submit(1, &submit_info, render_fence));
        vk::PresentInfoKHR const present_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &vk_swapchain,
            .pImageIndices = &swapchain_image_idx,
        };
        result = vk_queues.present_queue.presentKHR(present_info);
        if (result == vk::Result::eErrorOutOfDateKHR ||
            result == vk::Result::eSuboptimalKHR) {
            std::tie(vk_swapchain, vk_swapchain_images,
                vk_swapchain_image_views) = create_swapchain(vk_dev, vk_phy_dev,
                vk_surface, window, vk_queues);
        } else {
            CHECK(result == vk::Result::eSuccess, "");
        }
        glfwPollEvents();
        ++frame_idx;
    }
    ////////////////////
    ///* RenderLoop *///
    ////////////////////

    /////////////////
    ///* Cleanup *///
    /////////////////
    glfwTerminate();
    VK_CHECK(result, vk_dev.waitIdle());
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        vk_dev.destroyFence(render_fences[i]);
        vk_dev.destroySemaphore(render_semaphores[i]);
        vk_dev.destroySemaphore(present_semaphores[i]);
    }
    vk_dev.destroyPipeline(vk_graphics_pipeline);
    vk_dev.destroyPipelineLayout(vk_graphics_pipeline_layout);
    vk_dev.destroyCommandPool(vk_graphics_command_pool);
    for (auto const framebuffer : vk_framebuffers) {
        vk_dev.destroyFramebuffer(framebuffer);
    }
    vk_dev.destroyRenderPass(vk_render_pass);
    for (auto const view : vk_swapchain_image_views) {
        vk_dev.destroyImageView(view);
    }
    vk_dev.destroySwapchainKHR(vk_swapchain);
    vmaDestroyAllocator(vma_alloc);
    vk_dev.destroy();
    vk_inst.destroySurfaceKHR(vk_surface);
#if defined(VK_DBG)
    vk_inst.destroyDebugUtilsMessengerEXT(vk_dbg_messenger);
#endif
    vk_inst.destroy();
    /////////////////
    ///* Cleanup *///
    /////////////////

    return 0;
}

inline void glfw_error_callback(int error, const char* desc) {
    fmt::println("GLFW Error ({}): {}", error, desc);
}

GLFWwindow* glfw_create_window(int width, int height) {
    glfwInit();
    glfwSetErrorCallback(glfw_error_callback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window =
        glfwCreateWindow(width, height, "RayTracing", nullptr, nullptr);
    return window;
}
