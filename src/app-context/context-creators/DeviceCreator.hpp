#pragma once

#include "Common.hpp"
#include "utils/incl/Vulkan.hpp"

#include <vector>

class Logger;
namespace ContextCreator {

struct QueueSelection {
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
