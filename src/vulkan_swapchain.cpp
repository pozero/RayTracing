#include "vulkan_swapchain.h"
#include "vulkan_check.h"
#include "vulkan_device.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "GLFW/glfw3.h"
#pragma clang diagnostic pop

static bool swapchain_prepared = false;
static vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
static vk::SurfaceCapabilitiesKHR surface_capabilities{};
vk::SurfaceFormatKHR surface_format{};
vk::Extent2D swapchain_extent{};

void prepare_swapchain(
    vk::PhysicalDevice physical_dev, vk::SurfaceKHR surface) {
    vk::Result result;
    {
        std::vector<vk::SurfaceFormatKHR> candidates{};
        VK_CHECK_CREATE(
            result, candidates, physical_dev.getSurfaceFormatsKHR(surface));
        surface_format = candidates[0];
        for (auto const& format : candidates) {
            if (format.format == vk::Format::eR8G8B8A8Unorm &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                surface_format = format;
                break;
            } else if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                surface_format = format;
                break;
            }
        }
    }
    {
        std::vector<vk::PresentModeKHR> candidates{};
        VK_CHECK_CREATE(result, candidates,
            physical_dev.getSurfacePresentModesKHR(surface));
        for (auto const& mode : candidates) {
            if (mode == vk::PresentModeKHR::eMailbox) {
                present_mode = vk::PresentModeKHR::eMailbox;
                break;
            }
        }
    }
    VK_CHECK_CREATE(result, surface_capabilities,
        physical_dev.getSurfaceCapabilitiesKHR(surface));
    swapchain_prepared = true;
}

std::tuple<vk::SwapchainKHR, std::vector<vk::Image>, std::vector<vk::ImageView>>
    create_swapchain(vk::Device device, vk::SurfaceKHR surface,
        vulkan_queues const& queues) {
    CHECK(swapchain_prepared, "");
    vk::Result result;
    vk::SwapchainKHR swapchain;
    vk::SwapchainCreateInfoKHR swapchain_info{
        .surface = surface,
        .minImageCount = std::min(surface_capabilities.minImageCount + 1,
            surface_capabilities.maxImageCount),
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                      vk::ImageUsageFlagBits::eTransferSrc |
                      vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = vk::True,
    };
    std::array const sharing_queue{
        queues.present_queue_idx,
        queues.graphics_queue_idx,
    };
    if (queues.present_queue_idx != queues.graphics_queue_idx) {
        swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchain_info.queueFamilyIndexCount = (uint32_t) sharing_queue.size();
        swapchain_info.pQueueFamilyIndices = sharing_queue.data();
    }
    VK_CHECK_CREATE(
        result, swapchain, device.createSwapchainKHR(swapchain_info));
    std::vector<vk::Image> swapchain_images{};
    VK_CHECK_CREATE(
        result, swapchain_images, device.getSwapchainImagesKHR(swapchain));
    std::vector<vk::ImageView> swapchain_image_views(swapchain_images.size());
    for (uint32_t i = 0; i < swapchain_images.size(); ++i) {
        vk::ImageViewCreateInfo const image_view_info{
            .image = swapchain_images[i],
            .viewType = vk::ImageViewType::e2D,
            .format = surface_format.format,
            .components =
                {
                             vk::ComponentSwizzle::eIdentity,
                             vk::ComponentSwizzle::eIdentity,
                             vk::ComponentSwizzle::eIdentity,
                             vk::ComponentSwizzle::eIdentity,
                             },
            .subresourceRange =
                vk::ImageSubresourceRange{
                             .aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1,
                             },
        };
        VK_CHECK_CREATE(result, swapchain_image_views[i],
            device.createImageView(image_view_info, nullptr));
    }
    return std::make_tuple(swapchain, swapchain_images, swapchain_image_views);
}

void wait_window(vk::Device device, vk::PhysicalDevice physical_dev,
    vk::SurfaceKHR surface, struct GLFWwindow* window) {
    vk::Result result;
    do {
        VK_CHECK_CREATE(result, surface_capabilities,
            physical_dev.getSurfaceCapabilitiesKHR(surface));
        int width = 0;
        int height = 0;
        if (surface_capabilities.currentExtent.width == (uint32_t) -1 ||
            surface_capabilities.currentExtent.height == (uint32_t) -1) {
            glfwGetFramebufferSize(window, &width, &height);
            swapchain_extent = vk::Extent2D{
                .width = std::clamp((uint32_t) width,
                    surface_capabilities.minImageExtent.width,
                    surface_capabilities.maxImageExtent.width),
                .height = std::clamp((uint32_t) height,
                    surface_capabilities.minImageExtent.height,
                    surface_capabilities.maxImageExtent.height),
            };
            glfwWaitEvents();
        } else {
            swapchain_extent = surface_capabilities.currentExtent;
        }
    } while (swapchain_extent.width == 0 || swapchain_extent.height == 0);
    VK_CHECK(result, device.waitIdle());
}

vk::Result swapchain_acquire_next_image_wrapper(vk::Device device,
    vk::SwapchainKHR swapchain, uint64_t timeout, vk::Semaphore semaphore,
    vk::Fence fence, uint32_t* image_idx) {
    VkResult result = ::vk::defaultDispatchLoaderDynamic.vkAcquireNextImageKHR(
        device, swapchain, timeout, semaphore, fence, image_idx);
    return (vk::Result) result;
}

vk::Result swapchain_present_wrapper(
    vk::Queue queue, vk::PresentInfoKHR const& present_info) {
    VkResult result = ::vk::defaultDispatchLoaderDynamic.vkQueuePresentKHR(
        queue, reinterpret_cast<const VkPresentInfoKHR*>(&present_info));
    return (vk::Result) result;
}
