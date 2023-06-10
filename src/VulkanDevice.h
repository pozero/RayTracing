#pragma once

#include "VulkanHeader.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "vk_mem_alloc.h"
#pragma clang diagnostic pop

class VulkanDevice {
public:
    VulkanDevice() = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice(VulkanDevice const&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice const&) = delete;

public:
    VulkanDevice(std::span<const char*> inst_ext,
        std::span<const char*> inst_layer, struct GLFWwindow* window,
        std::span<const char*> dev_ext,
        const void* dev_creation_pnext) noexcept;

    ~VulkanDevice() noexcept;

private:
    vk::Instance m_instance;
#if defined(VK_DBG)
    vk::DebugUtilsMessengerEXT m_dbg_messenger;
#endif
    vk::SurfaceKHR m_surface;
    vk::PhysicalDevice m_physical_device;
    uint32_t m_graphics_queue_idx;
    uint32_t m_present_queue_idx;
    uint32_t m_compute_queue_idx;
    vk::Device m_device;
    vk::Queue m_graphics_queue;
    vk::Queue m_present_queue;
    vk::Queue m_compute_queue;
    VmaAllocator m_vma_alloc = VK_NULL_HANDLE;
};
