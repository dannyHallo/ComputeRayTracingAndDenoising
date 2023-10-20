#pragma once

#include "Common.hpp"

namespace ContextCreator {
void createSwapchain(VkSwapchainKHR &swapchain,
                     std::vector<VkImage> &swapchainImages,
                     std::vector<VkImageView> &swapchainImageViews,
                     VkFormat &swapchainImageFormat,
                     VkExtent2D &swapchainExtent, const VkSurfaceKHR &surface,
                     const VkDevice &device,
                     const VkPhysicalDevice &physicalDevice,
                     const QueueFamilyIndices &queueFamilyIndices);
} // namespace ContextCreator