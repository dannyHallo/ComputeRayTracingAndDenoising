#pragma once

#include "volk.h"

#include <cstdint>
#include <vector>

namespace ContextCreator {
// stores the indices of the each queue family, they might not overlap
struct QueueFamilyIndices {
  uint32_t graphicsFamily = 0;
  uint32_t presentFamily  = 0;
  uint32_t computeFamily  = 0;
  uint32_t transferFamily = 0;
};

struct SwapchainSupportDetails {
  // Basic surface capabilities (min/max number of images in swap chain, min/max
  // width and height of images) Surface formats (pixel format, color space)
  // Available presentation modes

  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};
} // namespace ContextCreator