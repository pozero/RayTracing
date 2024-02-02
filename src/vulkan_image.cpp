#include <vector>

#include "vulkan_image.h"
#include "vulkan_check.h"

static vma_image create_image(VmaAllocator vma_alloc, uint32_t width,
    uint32_t height, vk::Format format, std::vector<uint32_t> const& queues,
    vk::ImageCreateFlags flags, vk::ImageType type, vk::ImageUsageFlags usage,
    uint32_t levels, uint32_t layers, vk::ImageTiling tiling,
    vk::ImageLayout layout) {
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
}
