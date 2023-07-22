#pragma once

#include "memory/Buffer.h"
#include "memory/Image.h"
#include "scene/Material.h"
#include "utils/vulkan.h"

#include <memory>
#include <vector>

class ComputeMaterial : public Material {
  std::string mComputeShaderPath;

public:
  ComputeMaterial(const std::string &computeShaderPath) : Material(), mComputeShaderPath(computeShaderPath) {}

  void init();

  void bind(VkCommandBuffer &commandBuffer, size_t currentFrame);

private:
  void initComputePipeline(const std::string &computeShaderPath);
};