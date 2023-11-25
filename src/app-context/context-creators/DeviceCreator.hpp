#pragma once

#include "Common.hpp"
#include "utils/Vulkan.hpp"

#include <vector>

class Logger;
namespace ContextCreator {
void createDevice(Logger *logger, VkPhysicalDevice &physicalDevice, VkDevice &device,
                  QueueFamilyIndices &indices, VkQueue &graphicsQueue, VkQueue &presentQueue,
                  VkQueue &computeQueue, VkQueue &transferQueue, const VkInstance &instance,
                  VkSurfaceKHR surface, const std::vector<const char *> &requiredDeviceExtensions);
} // namespace ContextCreator
