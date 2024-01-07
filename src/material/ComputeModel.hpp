#pragma once

#include "utils/incl/Vulkan.hpp"

#include "ComputeMaterial.hpp"

#include <cmath>
#include <memory>

struct WorkGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

class Logger;
class VulkanApplicationContext;
class ComputeModel {
public:
  ComputeModel(std::unique_ptr<ComputeMaterial> material, size_t framesInFlight,
               WorkGroupSize workGroupSize)
      : _workGroupSize(workGroupSize), _computeMaterial(std::move(material)) {
    _computeMaterial->init(framesInFlight);
  }

  ComputeMaterial *getMaterial() { return _computeMaterial.get(); }

  void computeCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t threadCountX,
                      uint32_t threadCountY, uint32_t threadCountZ) {
    _computeMaterial->bind(commandBuffer, currentFrame);
    vkCmdDispatch(commandBuffer, std::ceil((float)threadCountX / (float)_workGroupSize.x),
                  std::ceil((float)threadCountY / (float)_workGroupSize.y),
                  std::ceil((float)threadCountZ / (float)_workGroupSize.z));
  }

private:
  WorkGroupSize _workGroupSize;

  std::unique_ptr<ComputeMaterial> _computeMaterial;
};
