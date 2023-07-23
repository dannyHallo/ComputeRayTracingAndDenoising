#pragma once

#include <memory>
#include <vector>

#include "app-context/VulkanApplicationContext.h"

// TODO: enrich this helper class
namespace RenderSystem {
VkCommandBuffer beginSingleTimeCommands();

void endSingleTimeCommands(VkCommandBuffer commandBuffer);
} // namespace RenderSystem