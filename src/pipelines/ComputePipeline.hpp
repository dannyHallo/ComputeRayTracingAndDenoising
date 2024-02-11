#pragma once

#include "pipelines/Pipeline.hpp"

#include <vector>

struct WorkGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

class ComputePipeline : public Pipeline {
public:
  ComputePipeline(VulkanApplicationContext *appContext, Logger *logger, std::string fileName,
                  WorkGroupSize workGroupSize, DescriptorSetBundle *descriptorSetBundle)
      : Pipeline(appContext, logger, std::move(fileName), descriptorSetBundle,
                 VK_SHADER_STAGE_COMPUTE_BIT),
        _workGroupSize(workGroupSize) {}

  void init() override;

  void recordCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t threadCountX,
                     uint32_t threadCountY, uint32_t threadCountZ);

  void recordIndirectCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                             VkBuffer indirectBuffer);

private:
  WorkGroupSize _workGroupSize;
};