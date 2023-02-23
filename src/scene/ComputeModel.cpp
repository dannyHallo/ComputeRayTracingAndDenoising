#pragma once

#include "ComputeModel.h"
#include "Mesh.h"
#include "memory/Buffer.h"
#include "utils/vulkan.h"

#include <vector>

namespace mcvkp {
ComputeModel::ComputeModel(std::shared_ptr<ComputeMaterial> material) : m_material(material) { m_material->init(); }

std::shared_ptr<ComputeMaterial> ComputeModel::getMaterial() { return m_material; }

void ComputeModel::computeCommand(VkCommandBuffer &commandBuffer, uint32_t currentFrame, uint32_t x, uint32_t y,
                                  uint32_t z) {
  m_material->bind(commandBuffer, currentFrame);
  vkCmdDispatch(commandBuffer, x, y, z);
}
} // namespace mcvkp
