#pragma once

#include "memory/Buffer.h"
#include "memory/Image.h"
#include "scene/Material.h"
#include "utils/vulkan.h"

#include <memory>
#include <vector>

class ComputeMaterial : public Material {
  std::string mComputeShaderPath;

public:
  ComputeMaterial(const std::string &computeShaderPath) : Material(), mComputeShaderPath(computeShaderPath) {}

  virtual void bind(VkCommandBuffer &commandBuffer, size_t currentFrame) override;

  // preset with shader stage flags
  virtual void addStorageImage(const std::shared_ptr<Image> &image,
                               VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT) override {
    Material::addStorageImage(image, shaderStageFlags);
  }

  // preset with shader stage flags
  virtual void addUniformBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle,
                                      VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT) override {
    Material::addUniformBufferBundle(bufferBundle, shaderStageFlags);
  }

  // preset with shader stage flags
  virtual void addStorageBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle,
                                      VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT) override {
    Material::addStorageBufferBundle(bufferBundle, shaderStageFlags);
  }

  virtual void init() override;

private:
  // must be called after initDescriptorSetLayout()
  void initComputePipeline(const std::string &computeShaderPath);
};