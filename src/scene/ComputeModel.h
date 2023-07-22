#pragma once

#include "ComputeMaterial.h"
#include "utils/vulkan.h"

#include <memory>
#include <vector>

class ComputeModel {
  std::shared_ptr<ComputeMaterial> mMaterial;

public:
  ComputeModel(std::shared_ptr<ComputeMaterial> material) : mMaterial(material) { mMaterial->init(); }

  std::shared_ptr<ComputeMaterial> getMaterial() { return mMaterial; }

  void computeCommand(VkCommandBuffer &commandBuffer, uint32_t currentFrame, uint32_t x, uint32_t y, uint32_t z) {
    mMaterial->bind(commandBuffer, currentFrame);
    vkCmdDispatch(commandBuffer, x, y, z);
  }
};