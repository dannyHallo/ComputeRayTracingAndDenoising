#pragma once

#include <memory>
#include <vector>

#include "utils/incl/Vulkan.hpp"

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                           VkCommandBuffer commandBuffer);