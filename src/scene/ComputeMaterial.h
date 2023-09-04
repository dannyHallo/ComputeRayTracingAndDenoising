#pragma once

#include "memory/Buffer.h"
#include "memory/Image.h"
#include "scene/Material.h"
#include "utils/vulkan.h"

#include <string>

class ComputeMaterial : public Material {
  std::string mComputeShaderPath;

public:
  ComputeMaterial(std::string computeShaderPath)
      : Material(VK_SHADER_STAGE_COMPUTE_BIT), mComputeShaderPath(std::move(computeShaderPath)) {}

  void bind(VkCommandBuffer commandBuffer, size_t currentFrame) override;

  void init() override;

private:
  // must be called after initDescriptorSetLayout()
  void initComputePipeline(const std::string &computeShaderPath);
};