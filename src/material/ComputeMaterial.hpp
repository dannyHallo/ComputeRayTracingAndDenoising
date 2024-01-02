#pragma once

#include "material/Material.hpp"
#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "utils/incl/Vulkan.hpp"

#include <string>

class ComputeMaterial : public Material {
public:
  ComputeMaterial(VulkanApplicationContext *appContext, Logger *logger,
                  std::string computeShaderPath)
      : Material(appContext, logger, VK_SHADER_STAGE_COMPUTE_BIT),
        _computeShaderPath(std::move(computeShaderPath)) {}

private:
  std::string _computeShaderPath;

  void _createPipeline() override;
};