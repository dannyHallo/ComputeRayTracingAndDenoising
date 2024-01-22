#pragma once

#include "material/Material.hpp"

#include <vector>

class ComputeMaterial : public Material {
public:
  ComputeMaterial(VulkanApplicationContext *appContext, Logger *logger,
                  std::vector<uint32_t> &&shaderCode, DescriptorSetBundle *descriptorSetBundle)
      : Material(appContext, logger, std::move(shaderCode), descriptorSetBundle,
                 VK_SHADER_STAGE_COMPUTE_BIT) {}

private:
  void _createPipeline() override;
};