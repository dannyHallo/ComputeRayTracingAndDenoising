#pragma once

#include "utils/incl/Vulkan.hpp"

#include "ComputeMaterial.hpp"

#include <cmath>
#include <memory>

class Logger;
class VulkanApplicationContext;
class ComputeModel {
public:
  ComputeModel(std::unique_ptr<ComputeMaterial> material, uint32_t localSizeX, uint32_t localSizeY,
               uint32_t localSizeZ)
      : _localSizeX(localSizeX), _localSizeY(localSizeY), _localSizeZ(localSizeZ),
        _computeMaterial(std::move(material)) {
    _computeMaterial->init();
  }

  ComputeMaterial *getMaterial() { return _computeMaterial.get(); }

  void computeCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t threadCountX,
                      uint32_t threadCountY, uint32_t threadCountZ) {
    _computeMaterial->bind(commandBuffer, currentFrame);
    vkCmdDispatch(commandBuffer, std::ceil(threadCountX / static_cast<float>(_localSizeX)),
                  std::ceil(threadCountY / static_cast<float>(_localSizeY)),
                  std::ceil(threadCountZ / static_cast<float>(_localSizeZ)));
  }

private:
  uint32_t _localSizeX;
  uint32_t _localSizeY;
  uint32_t _localSizeZ;

  std::unique_ptr<ComputeMaterial> _computeMaterial;
};
