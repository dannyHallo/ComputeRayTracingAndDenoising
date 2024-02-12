#pragma once

#include "pipeline/Pipeline.hpp"

#include <memory>
#include <vector>

struct WorkGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

class ShaderCompiler;

class ComputePipeline : public Pipeline {
public:
  ComputePipeline(VulkanApplicationContext *appContext, Logger *logger, Scheduler *scheduler,
                  std::string shaderFileName, WorkGroupSize workGroupSize,
                  DescriptorSetBundle *descriptorSetBundle,
                  ShaderChangeListener *shaderChangeListener = nullptr);

  ~ComputePipeline() override;

  // disable copy and move
  ComputePipeline(const ComputePipeline &)            = delete;
  ComputePipeline &operator=(const ComputePipeline &) = delete;
  ComputePipeline(ComputePipeline &&)                 = delete;
  ComputePipeline &operator=(ComputePipeline &&)      = delete;

  void build(bool allowCache) override;

  void recordCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t threadCountX,
                     uint32_t threadCountY, uint32_t threadCountZ);

  void recordIndirectCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                             VkBuffer indirectBuffer);

private:
  WorkGroupSize _workGroupSize;

  std::unique_ptr<ShaderCompiler> _shaderCompiler;
};