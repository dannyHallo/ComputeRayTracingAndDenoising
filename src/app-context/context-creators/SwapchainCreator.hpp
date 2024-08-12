#pragma once

#include "Common.hpp"

class Logger;
namespace ContextCreator {
void createSwapchain(Logger *logger, bool isFramerateLimited, VkSwapchainKHR &swapchain,
                     std::vector<VkImage> &swapchainImages,
                     std::vector<VkImageView> &swapchainImageViews,
                     VkSurfaceFormatKHR &swapchainImageFormat, VkExtent2D &swapchainExtent,
                     const VkSurfaceKHR &surface, const VkDevice &device,
                     const VkPhysicalDevice &physicalDevice,
                     const QueueFamilyIndices &queueFamilyIndices);
} // namespace ContextCreator