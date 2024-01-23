#pragma once

#include "material/Pipeline.hpp"

#include <vector>

struct WorkGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

class ComputePipeline : public Pipeline {
public:
  ComputePipeline(VulkanApplicationContext *appContext, Logger *logger,
                  std::vector<uint32_t> &&shaderCode, WorkGroupSize workGroupSize,
                  DescriptorSetBundle *descriptorSetBundle)
      : Pipeline(appContext, logger, std::move(shaderCode), descriptorSetBundle,
                 VK_SHADER_STAGE_COMPUTE_BIT),
        _workGroupSize(workGroupSize) {}

  void create() override;

  void recordCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t threadCountX,
                     uint32_t threadCountY, uint32_t threadCountZ);

private:
  WorkGroupSize _workGroupSize;
};