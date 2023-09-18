#pragma once

#include "utils/vulkan.h"

#include <cstdint>
#include <vector>

// stores the indices of the each queue family, they might not overlap
struct QueueFamilyIndices {
  uint32_t graphicsFamily = -1;
  uint32_t presentFamily  = -1;
  uint32_t computeFamily  = -1;
  uint32_t transferFamily = -1;
};

struct SwapchainSupportDetails {
  // Basic surface capabilities (min/max number of images in swap chain, min/max width and height
  // of images) Surface formats (pixel format, color space) Available presentation modes

  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};