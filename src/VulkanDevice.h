#pragma once

#include "VulkanHeader.h"

class VulkanDevice {
public:
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice(VulkanDevice const&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice const&) = delete;

public:
    VulkanDevice() noexcept;

    ~VulkanDevice() noexcept;

private:
    vk::Instance m_instance;
};
