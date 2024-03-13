#include <vector>

#include "check.h"
#include "vulkan/vulkan_image.h"
#include "asset/texture.h"

#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
static std::vector<vma_image> staging_images{};

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
    return vma_image{
        image, allocation, width, height, (vk::Format) format, nullptr, {}};
}

vma_image create_texture2d_simple(vk::Device device, VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    vk::Format format, std::vector<uint32_t> const& queues,
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
    image.primary_view = view;
    return image;
}

vma_image create_cubemap(vk::Device device, VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    vk::Format format, std::vector<uint32_t> const& queues) {
    vk::Result result;
    vk::ImageView view;
    vma_image image = create_image(vma_alloc, width, height, format, queues,
        vk::ImageCreateFlagBits::eCubeCompatible, vk::ImageType::e2D,
        vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eStorage,
        1, 6, vk::ImageTiling::eOptimal, vk::ImageLayout::eUndefined,
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
                                      .layerCount = 6},
    };
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eNone,
        vk::PipelineStageFlagBits::eNone, vk::DependencyFlags{}, 0, nullptr, 0,
        nullptr, 1, &image_barrier);
    vk::ImageViewCreateInfo const view_info{
        .image = image.image,
        .viewType = vk::ImageViewType::eCube,
        .format = format,
        .components = vk::ComponentMapping{.r = vk::ComponentSwizzle::eIdentity,
                                           .g = vk::ComponentSwizzle::eIdentity,
                                           .b = vk::ComponentSwizzle::eIdentity,
                                           .a = vk::ComponentSwizzle::eIdentity},
        .subresourceRange =
            vk::ImageSubresourceRange{
                                           .aspectMask = vk::ImageAspectFlagBits::eColor,
                                           .baseMipLevel = 0,
                                           .levelCount = 1,
                                           .baseArrayLayer = 0,
                                           .layerCount = 6},
    };
    VK_CHECK_CREATE(result, view, device.createImageView(view_info));
    image.primary_view = view;
    return image;
}

vma_image create_host_image(VmaAllocator vma_alloc,
    vk::CommandBuffer command_buffer, uint32_t width, uint32_t height,
    vk::Format format) {
    VkImage handle;
    VmaAllocation allocation;
    VkImageCreateInfo const image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = (VkFormat) format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo const allocation_create_info{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };
    VmaAllocationInfo allocation_ret{};
    CHECK(vmaCreateImage(vma_alloc, &image_info, &allocation_create_info,
              &handle, &allocation, &allocation_ret) == VK_SUCCESS,
        "");
    vma_image ret{handle, allocation, width, height, format,
        allocation_ret.pMappedData, {}};
    vk::ImageMemoryBarrier const image_barrier{
        .srcAccessMask = vk::AccessFlagBits::eNone,
        .dstAccessMask = vk::AccessFlagBits::eNone,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = ret.image,
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
    return ret;
}

#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
void update_host_image(VmaAllocator vma_alloc, vma_image const& image,
    texture_data const& texture_data) {
    if (image.format == vk::Format::eR8G8B8A8Unorm) {
        using image_data_format = uint8_t;
        image_data_format const* src =
            reinterpret_cast<image_data_format const*>(texture_data.data);
        image_data_format* dst =
            reinterpret_cast<image_data_format*>(image.mapped);
        if (texture_data.channel == texture_channel::rgb) {
            for (uint32_t pixel = 0;
                 pixel < texture_data.width * texture_data.height; ++pixel) {
                dst[4 * pixel + 0] = src[3 * pixel + 0];
                dst[4 * pixel + 1] = src[3 * pixel + 1];
                dst[4 * pixel + 2] = src[3 * pixel + 2];
                dst[4 * pixel + 3] = 255;
            }
        } else if (texture_data.channel == texture_channel::rgba) {
            std::copy(src, &src[texture_data.width * texture_data.height * 4],
                reinterpret_cast<image_data_format*>(image.mapped));
        }
    } else if (image.format == vk::Format::eR32G32B32A32Sfloat) {
        using image_data_format = float;
        image_data_format const* src =
            reinterpret_cast<image_data_format const*>(texture_data.data);
        image_data_format* dst =
            reinterpret_cast<image_data_format*>(image.mapped);
        if (texture_data.channel == texture_channel::rgb) {
            for (uint32_t pixel = 0;
                 pixel < texture_data.width * texture_data.height; ++pixel) {
                dst[4 * pixel + 0] = src[3 * pixel + 0];
                dst[4 * pixel + 1] = src[3 * pixel + 1];
                dst[4 * pixel + 2] = src[3 * pixel + 2];
                dst[4 * pixel + 3] = 1.0f;
            }
        } else if (texture_data.channel == texture_channel::rgba) {
            std::copy(src, &src[texture_data.width * texture_data.height * 4],
                reinterpret_cast<image_data_format*>(image.mapped));
        }
    } else {
        CHECK(false, "Unsupported host image format: {}",
            vk::to_string(image.format));
    }
    vmaFlushAllocation(vma_alloc, image.allocation, 0, VK_WHOLE_SIZE);
}

void update_texture2d_simple(VmaAllocator vma_alloc, vma_image const& image,
    vk::CommandBuffer command_buffer, texture_data const& texture_data) {
    vma_image staging;
    vk::ImageSubresourceLayers const whole_layer{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    vk::Format const host_format =
        texture_data.format == texture_format::unorm ?
            vk::Format::eR8G8B8A8Unorm :
            vk::Format::eR32G32B32A32Sfloat;
    staging = create_host_image(vma_alloc, command_buffer, texture_data.width,
        texture_data.height, host_format);
    staging_images.push_back(staging);
    update_host_image(vma_alloc, staging, texture_data);
    bool const need_reformat = image.format != staging.format;
    bool const need_resize = image.width != texture_data.width ||
                             image.height != texture_data.height;
    if (need_reformat || need_resize) {
        std::array const src_offsets{
            vk::Offset3D{  0,                             0, 0                         },
            vk::Offset3D{
                         (int32_t) texture_data.width, (int32_t) texture_data.height, 1},
        };
        std::array const dst_offsets{
            vk::Offset3D{                    0,                      0, 0},
            vk::Offset3D{(int32_t) image.width, (int32_t) image.height, 1},
        };
        vk::ImageBlit const blit_info{
            .srcSubresource = whole_layer,
            .srcOffsets = src_offsets,
            .dstSubresource = whole_layer,
            .dstOffsets = dst_offsets,
        };
        command_buffer.blitImage(staging.image, vk::ImageLayout::eGeneral,
            image.image, vk::ImageLayout::eGeneral, 1, &blit_info,
            vk::Filter::eNearest);
    } else {
        vk::ImageCopy const image_copy{
            .srcSubresource = whole_layer,
            .srcOffset = vk::Offset3D{          0,            0, 0},
            .dstSubresource = whole_layer,
            .dstOffset = vk::Offset3D{          0,            0, 0},
            .extent = vk::Extent3D{image.width, image.height, 1},
        };
        command_buffer.copyImage(staging.image, vk::ImageLayout::eGeneral,
            image.image, vk::ImageLayout::eGeneral, 1, &image_copy);
    }
}

vk::Sampler create_default_sampler(vk::Device device) {
    vk::Result result;
    vk::Sampler sampler;
    vk::SamplerCreateInfo const sampler_info{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
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

void destroy_image(
    vk::Device device, VmaAllocator vma_alloc, vma_image const& image) {
    vmaDestroyImage(vma_alloc, image.image, image.allocation);
    if (image.primary_view) {
        device.destroyImageView(image.primary_view);
    }
}

void cleanup_staging_image(VmaAllocator vma_alloc) {
    for (auto const& staging : staging_images) {
        vmaDestroyImage(vma_alloc, staging.image, staging.allocation);
    }
}
