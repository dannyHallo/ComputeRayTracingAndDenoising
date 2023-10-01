#pragma once

#include <memory>
#include <vector>

#include "utils/Vulkan.h"

namespace RenderSystem {
VkCommandBuffer beginSingleTimeCommands(VkDevice device,
                                        VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool,
                           VkQueue queue, VkCommandBuffer commandBuffer);
} // namespace RenderSystem