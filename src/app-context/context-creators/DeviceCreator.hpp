#pragma once

#include "Common.hpp"
#include "volk.h"

#include <vector>

class Logger;
namespace ContextCreator {

struct QueueSelection {
  uint32_t graphicsQueueIndex;
  uint32_t presentQueueIndex;
  uint32_t computeQueueIndex;
  uint32_t transferQueueIndex;

  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkQueue computeQueue;
  VkQueue transferQueue;
};

void createDevice(Logger *logger, VkPhysicalDevice &physicalDevice, VkDevice &device,
                  QueueFamilyIndices &indices, QueueSelection &queueSelection,
                  const VkInstance &instance, VkSurfaceKHR surface,
                  const std::vector<const char *> &requiredDeviceExtensions);
} // namespace ContextCreator
