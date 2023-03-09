#pragma once

#include "memory/Buffer.h"
#include "memory/Image.h"
#include "scene/Material.h"
#include "utils/vulkan.h"

#include <memory>
#include <vector>

class ComputeMaterial : public Material {
public:
  ComputeMaterial(const std::string &computeShaderPath);

  void init();

  void bind(VkCommandBuffer &commandBuffer, size_t currentFrame);

private:
  void __initComputePipeline(const std::string &computeShaderPath);

private:
  std::string m_computeShaderPath;
};