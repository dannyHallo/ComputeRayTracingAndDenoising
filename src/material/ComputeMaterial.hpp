#pragma once

#include "material/Material.hpp"
#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "utils/incl/Vulkan.hpp"

#include <string>

class ComputeMaterial : public Material {
public:
  ComputeMaterial(VulkanApplicationContext *appContext, std::string computeShaderPath)
      : Material(appContext, VK_SHADER_STAGE_COMPUTE_BIT),
        _computeShaderPath(std::move(computeShaderPath)) {}

  void bind(VkCommandBuffer commandBuffer, size_t currentFrame) override;

  void init(Logger *logger) override;

private:
  std::string _computeShaderPath;

  void _createComputePipeline();
};