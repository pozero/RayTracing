#include <vector>

#include "vulkan_image.h"
#include "vulkan_check.h"

vma_image create_image(VmaAllocator vma_alloc, uint32_t width, uint32_t height,
    vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageCreateFlags flags, vk::ImageType type, vk::ImageUsageFlags usage,
    uint32_t levels, uint32_t layers, vk::ImageTiling tiling,
    vk::ImageLayout layout, VmaAllocationCreateFlags alloc_flags) {
    VkImage image;
    VmaAllocation allocation;
    VkImageCreateInfo const image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = (VkImageCreateFlags) flags,
        .imageType = (VkImageType) type,
        .format = (VkFormat) format,
        .extent =
            VkExtent3D{
                       .width = width,
                       .height = height,
                       .depth = 1,
                       },
        .mipLevels = levels,
        .arrayLayers = layers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = (VkImageTiling) tiling,
        .usage = (VkImageUsageFlags) usage,
        .sharingMode = queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT :
                                           VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = (uint32_t) queues.size(),
        .pQueueFamilyIndices = queues.data(),
        .initialLayout = (VkImageLayout) layout,
    };
    VmaAllocationCreateInfo const allocation_info{
        .flags = alloc_flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    CHECK(vmaCreateImage(vma_alloc, &image_info, &allocation_info, &image,
              &allocation, nullptr) == VK_SUCCESS,
        "");
    return vma_image{image, allocation, width, height, (vk::Format) format};
}

std::pair<vma_image, vk::ImageView> create_texture2d_simple(vk::Device device,
    VmaAllocator vma_alloc, vk::CommandBuffer command_buffer, uint32_t width,
    uint32_t height, vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageUsageFlags usage) {
    vk::Result result;
    vk::ImageView view;
    vma_image image = create_image(vma_alloc, width, height, format, queues,
        vk::ImageCreateFlags{}, vk::ImageType::e2D,
        vk::ImageUsageFlagBits::eSampled | usage, 1, 1,
        vk::ImageTiling::eOptimal, vk::ImageLayout::eUndefined,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    vk::ImageMemoryBarrier const image_barrier{
        .srcAccessMask = vk::AccessFlagBits::eNone,
        .dstAccessMask = vk::AccessFlagBits::eNone,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image.image,
        .subresourceRange =
            vk::ImageSubresourceRange{
                                      .aspectMask = vk::ImageAspectFlagBits::eColor,
                                      .baseMipLevel = 0,
                                      .levelCount = 1,
                                      .baseArrayLayer = 0,
                                      .layerCount = 1,
                                      },
    };
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eNone,
        vk::PipelineStageFlagBits::eNone, vk::DependencyFlags{}, 0, nullptr, 0,
        nullptr, 1, &image_barrier);
    vk::ImageViewCreateInfo const view_info{
        .image = image.image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .components =
            vk::ComponentMapping{
                                 .r = vk::ComponentSwizzle::eIdentity,
                                 .g = vk::ComponentSwizzle::eIdentity,
                                 .b = vk::ComponentSwizzle::eIdentity,
                                 .a = vk::ComponentSwizzle::eIdentity,
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
    VK_CHECK_CREATE(result, view, device.createImageView(view_info));
    return std::make_pair(image, view);
}

vk::Sampler create_default_sampler(vk::Device device) {
    vk::Result result;
    vk::Sampler sampler;
    vk::SamplerCreateInfo const sampler_info{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::False,
        .maxAnisotropy = 16.0f,
        .compareEnable = vk::False,
        .minLod = 0.0f,
        .maxLod = 12.0f,
        .borderColor = vk::BorderColor::eFloatOpaqueBlack,
        .unnormalizedCoordinates = vk::False,
    };
    VK_CHECK_CREATE(result, sampler, device.createSampler(sampler_info));
    return sampler;
}

void destroy_image(VmaAllocator vma_alloc, vma_image const& image) {
    vmaDestroyImage(vma_alloc, image.image, image.allocation);
}
