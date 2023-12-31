#pragma once

#include "utils/incl/Vulkan.hpp"

#include "ComputeMaterial.hpp"

#include <memory>

class Logger;
class ComputeModel {
  std::unique_ptr<ComputeMaterial> mComputeMaterial;

public:
  ComputeModel(Logger *logger, std::unique_ptr<ComputeMaterial> material)
      : mComputeMaterial(std::move(material)) {
    mComputeMaterial->init(logger);
  }

  ComputeMaterial *getMaterial() { return mComputeMaterial.get(); }

  void computeCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t x, uint32_t y,
                      uint32_t z) {
    mComputeMaterial->bind(commandBuffer, currentFrame);
    vkCmdDispatch(commandBuffer, x, y, z);
  }
};