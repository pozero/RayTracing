#include <span>
#include <vector>

#include "check.h"
#include "vulkan/vulkan_buffer.h"

#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
static std::vector<vma_buffer> staging_buffers{};
static vma_buffer dummy_uniform_buffer{};
static vma_buffer dummy_storage_buffer{};

static bool is_dummy(vma_buffer const& buffer) {
    return buffer.buffer == dummy_uniform_buffer.buffer ||
           buffer.buffer == dummy_storage_buffer.buffer;
}

vma_buffer create_buffer(VmaAllocator vma_alloc, uint32_t size,
    std::vector<uint32_t> const& queues, VkBufferCreateFlags buffer_flags,
    VkBufferUsageFlags usage_flag, VmaAllocationCreateFlags alloc_flags) {
    // dummy buffer
    if (size == 0) {
        if ((usage_flag & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) != 0) {
            return dummy_uniform_buffer;
        } else if ((usage_flag & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0) {
            return dummy_storage_buffer;
        } else {
            CHECK(false, "");
        }
    }
    VkBuffer buffer;
    VmaAllocation alloc;
    VkBufferCreateInfo const buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = buffer_flags,
        .size = size,
        .usage = usage_flag,
        .sharingMode = queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT :
                                           VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = (uint32_t) queues.size(),
        .pQueueFamilyIndices = queues.data(),
    };
    VmaAllocationCreateInfo const allocation_info{
        .flags = alloc_flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VmaAllocationInfo allocation_result{};
    CHECK(vmaCreateBuffer(vma_alloc, &buffer_info, &allocation_info, &buffer,
              &alloc, &allocation_result) == VK_SUCCESS,
        "");
    return vma_buffer{
        buffer, alloc, (uint8_t*) allocation_result.pMappedData, size};
}

void destory_buffer(VmaAllocator vma_alloc, vma_buffer const& buffer) {
    if (is_dummy(buffer)) {
        return;
    }
    vmaDestroyBuffer(vma_alloc, buffer.buffer, buffer.allocation);
}

vma_buffer create_gpu_only_buffer(VmaAllocator vma_alloc, uint32_t size,
    std::vector<uint32_t> const& queues, vk::BufferUsageFlags usage) {
    return create_buffer(vma_alloc, size, queues, 0,
        (VkBufferUsageFlags) (usage | vk::BufferUsageFlagBits::eTransferDst),
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
}

vma_buffer create_staging_buffer(VmaAllocator vma_alloc, uint32_t size,
    std::vector<uint32_t> const& queues) {
    return create_buffer(vma_alloc, size, queues, 0,
        (VkBufferUsageFlags) (vk::BufferUsageFlagBits::eTransferSrc),
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

vma_buffer create_frequent_readwrite_buffer(VmaAllocator vma_alloc,
    uint32_t size, std::vector<uint32_t> const& queues,
    vk::BufferUsageFlags usage) {
    return create_buffer(vma_alloc, size, queues, 0,
        (VkBufferUsageFlags) (usage | vk::BufferUsageFlagBits::eTransferSrc |
                              vk::BufferUsageFlagBits::eTransferDst),
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
void update_buffer(VmaAllocator vma_alloc, vk::CommandBuffer command_buffer,
    vma_buffer const& buffer, std::span<const uint8_t> data, uint32_t offset) {
    if (is_dummy(buffer)) {
        return;
    }
    if (buffer.mapped) {
        std::copy(data.begin(), data.end(), &buffer.mapped[offset]);
        vmaFlushAllocation(
            vma_alloc, buffer.allocation, offset, (uint32_t) data.size());
    } else {
        vma_buffer staging = create_staging_buffer(
            vma_alloc, (uint32_t) data.size(), {vk::QueueFamilyIgnored});
        staging_buffers.push_back(staging);
        std::copy(data.begin(), data.end(), staging.mapped);
        vmaFlushAllocation(
            vma_alloc, buffer.allocation, 0, (uint32_t) data.size());
        vk::BufferCopy const copy_info{
            .srcOffset = 0,
            .dstOffset = offset,
            .size = (uint32_t) data.size(),
        };
        command_buffer.copyBuffer(staging.buffer, buffer.buffer, 1, &copy_info);
    }
}

void cleanup_staging_buffer(VmaAllocator vma_alloc) {
    for (auto const staging : staging_buffers) {
        destory_buffer(vma_alloc, staging);
    }
}

void create_dummy_buffer(VmaAllocator vma_alloc) {
    VkBuffer buffer;
    VmaAllocation alloc;
    VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = {},
        .size = 4,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    VmaAllocationCreateInfo const allocation_info{
        .flags = {},
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VmaAllocationInfo allocation_result{};
    CHECK(vmaCreateBuffer(vma_alloc, &buffer_info, &allocation_info, &buffer,
              &alloc, &allocation_result) == VK_SUCCESS,
        "");
    buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    dummy_uniform_buffer = vma_buffer{buffer, alloc, nullptr, 1};
    CHECK(vmaCreateBuffer(vma_alloc, &buffer_info, &allocation_info, &buffer,
              &alloc, &allocation_result) == VK_SUCCESS,
        "");
    dummy_storage_buffer = vma_buffer{buffer, alloc, nullptr, 1};
}

void destroy_dummy_buffer(VmaAllocator vma_alloc) {
    vmaDestroyBuffer(vma_alloc, dummy_uniform_buffer.buffer,
        dummy_uniform_buffer.allocation);
    vmaDestroyBuffer(vma_alloc, dummy_storage_buffer.buffer,
        dummy_storage_buffer.allocation);
}
