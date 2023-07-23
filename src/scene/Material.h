#pragma once

#include "memory/Buffer.h"
#include "memory/Image.h"
#include "utils/vulkan.h"

#include <memory>
#include <vector>

// A template struct to hold a descriptor and the shader stage flags.
template <typename T> struct Descriptor {
  std::shared_ptr<T> data;
  VkShaderStageFlags shaderStageFlags;
};

class Material {

public:
  Material() : mSwapchainSize(static_cast<uint32_t>(vulkanApplicationContext.getSwapchainSize())) {}

  virtual ~Material();

  virtual void addStorageImage(const std::shared_ptr<Image> &image, VkShaderStageFlags shaderStageFlags);
  virtual void addUniformBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle,
                                      VkShaderStageFlags shaderStageFlags);
  virtual void addStorageBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle,
                                      VkShaderStageFlags shaderStageFlags);

  const std::vector<Descriptor<BufferBundle>> &getUniformBufferBundles() const {
    return mUniformBufferBundleDescriptors;
  }

  const std::vector<Descriptor<BufferBundle>> &getStorageBufferBundles() const {
    return mStorageBufferBundleDescriptors;
  }

  const std::vector<Descriptor<Image>> &getStorageImages() const { return mStorageImageDescriptors; }

  // binding should be done in model, must be overridden
  virtual void bind(VkCommandBuffer &commandBuffer, size_t currentFrame) = 0;

protected:
  std::vector<Descriptor<BufferBundle>> mUniformBufferBundleDescriptors; // buffer bundles for uniform data
  std::vector<Descriptor<BufferBundle>> mStorageBufferBundleDescriptors; // buffer bundles for storage data
  // std::vector<Descriptor<Texture>> mTextureDescriptors;
  std::vector<Descriptor<Image>> mStorageImageDescriptors; // images for storage data

  std::string mVertexShaderPath;
  std::string mFragmentShaderPath;

  uint32_t mSwapchainSize;

  VkPipeline mPipeline;
  VkPipelineLayout mPipelineLayout;

  VkDescriptorPool mDescriptorPool;
  std::vector<VkDescriptorSet> mDescriptorSets;
  VkDescriptorSetLayout mDescriptorSetLayout;

  // creates mDescriptorSetLayout from resources added to the material
  void initDescriptorSetLayout();

  // allocates pool sizes and creates mDescriptorPool
  void initDescriptorPool();

  void initDescriptorSets();

  VkShaderModule createShaderModule(const std::vector<char> &code);

  // late initialization during model creation, must be overridden
  virtual void init() = 0;
};
