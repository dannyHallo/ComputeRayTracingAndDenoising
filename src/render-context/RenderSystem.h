#pragma once

#include <memory>
#include <vector>

#include "app-context/VulkanApplicationContext.h"
// #include "scene/Scene.h"

namespace RenderSystem {
void allocateCommandBuffers(std::vector<VkCommandBuffer> &commandBuffers, uint32_t numBuffers);

void beginCommandBuffer(const VkCommandBuffer &commandBuffer);

void endCommandBuffer(const VkCommandBuffer &commandBuffer);

VkCommandBuffer beginSingleTimeCommands();

void endSingleTimeCommands(VkCommandBuffer commandBuffer);

void submit(const VkCommandBuffer *commandBuffer, const size_t &numWaitSemaphores, const VkSemaphore *waitSemaphores,
            const VkPipelineStageFlags *waitStages, const VkSemaphore *signalSemaphores, VkFence &fence);

void present(const uint32_t &imageIndex, const VkSemaphore *semaphores, const size_t &numSemaphores);
} // namespace RenderSystem