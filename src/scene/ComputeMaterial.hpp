#pragma once

#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "scene/Material.hpp"
#include "utils/incl/Vulkan.hpp"

#include <string>

class ComputeMaterial : public Material {
  std::string mComputeShaderPath;

public:
  ComputeMaterial(std::string computeShaderPath)
      : Material(VK_SHADER_STAGE_COMPUTE_BIT), mComputeShaderPath(std::move(computeShaderPath)) {}

  void bind(VkCommandBuffer commandBuffer, size_t currentFrame) override;

  void init(Logger *logger) override;

private:
  // must be called after initDescriptorSetLayout()
  void initComputePipeline(const std::string &computeShaderPath);
};