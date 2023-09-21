#pragma once

#include "Common.h"

namespace SwapchainCreator {
void create(VkSwapchainKHR &swapchain, std::vector<VkImage> &swapchainImages,
            std::vector<VkImageView> &swapchainImageViews,
            VkFormat &swapchainImageFormat, VkExtent2D &swapchainExtent,
            const VkSurfaceKHR &surface, const VkDevice &device,
            const VkPhysicalDevice &physicalDevice,
            const QueueFamilyIndices &queueFamilyIndices);
}