#pragma once

#include "Common.h"
#include "utils/Vulkan.h"

#include <vector>

namespace DeviceCreator {
void create(VkPhysicalDevice &physicalDevice, VkDevice &device,
            QueueFamilyIndices &indices, VkQueue &graphicsQueue,
            VkQueue &presentQueue, VkQueue &computeQueue,
            VkQueue &transferQueue, const VkInstance &instance,
            VkSurfaceKHR surface,
            const std::vector<const char *> &requiredDeviceExtensions);
} // namespace DeviceCreator
