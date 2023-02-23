#pragma once

#include "ComputeMaterial.h"
#include "Mesh.h"
#include "memory/Buffer.h"
#include "utils/vulkan.h"

#include <vector>

namespace mcvkp {

class ComputeModel {
public:
  ComputeModel(std::shared_ptr<ComputeMaterial> material);

  std::shared_ptr<ComputeMaterial> getMaterial();
  void computeCommand(VkCommandBuffer &commandBuffer, uint32_t currentFrame, uint32_t x, uint32_t y, uint32_t z);

private:
  std::shared_ptr<ComputeMaterial> m_material;
};
} // namespace mcvkp
