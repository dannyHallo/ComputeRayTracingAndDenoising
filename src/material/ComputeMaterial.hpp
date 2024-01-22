#pragma once

#include "material/Material.hpp"
#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "utils/incl/Vulkan.hpp"

#include <string>
#include <vector>

class ComputeMaterial : public Material {
public:
  ComputeMaterial(VulkanApplicationContext *appContext, Logger *logger,
                  std::vector<uint32_t> &&code)
      : Material(appContext, logger, VK_SHADER_STAGE_COMPUTE_BIT), _code(std::move(code)) {}

private:
  std::vector<uint32_t> _code;

  void _createPipeline() override;
};