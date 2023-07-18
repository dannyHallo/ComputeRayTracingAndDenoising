#pragma once

#include "memory/Buffer.h"
#include "memory/Image.h"
#include "utils/vulkan.h"

#include <memory>
#include <vector>

template <typename T> struct Descriptor {
  std::shared_ptr<T> data;
  VkShaderStageFlags shaderStageFlags;
};

class Material {
protected:
  std::vector<Descriptor<BufferBundle>> mUniformBufferBundleDescriptors;
  std::vector<Descriptor<BufferBundle>> mStorageBufferBundleDescriptors;
  // std::vector<Descriptor<Texture>> mTextureDescriptors;
  std::vector<Descriptor<Image>> mStorageImageDescriptors;

  std::string mVertexShaderPath;
  std::string mFragmentShaderPath;

  bool mInitialized;

  uint32_t mDescriptorSetsSize;

  VkPipeline mPipeline;
  VkPipelineLayout mPipelineLayout;

  VkDescriptorPool mDescriptorPool;
  std::vector<VkDescriptorSet> mDescriptorSets;
  VkDescriptorSetLayout mDescriptorSetLayout;

public:
  // Material(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);

  Material() : mDescriptorSetsSize(static_cast<uint32_t>(vulkanApplicationContext.getSwapchainSize())) {}

  virtual ~Material();

  // void addTexture(const std::shared_ptr<Texture> &texture, VkShaderStageFlags shaderStageFlags);

  void addStorageImage(const std::shared_ptr<Image> &image, VkShaderStageFlags shaderStageFlags);

  void addUniformBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle, VkShaderStageFlags shaderStageFlags);

  void addStorageBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle, VkShaderStageFlags shaderStageFlags);

  const std::vector<Descriptor<BufferBundle>> &getUniformBufferBundles() const {
    return mUniformBufferBundleDescriptors;
  }

  const std::vector<Descriptor<BufferBundle>> &getStorageBufferBundles() const {
    return mStorageBufferBundleDescriptors;
  }

  // const std::vector<Descriptor<Texture>> &getTextures() const{
  //   return mTextureDescriptors;
  // }

  const std::vector<Descriptor<Image>> &getStorageImages() const { return mStorageImageDescriptors; }

  // Initialize material when adding to a scene.
  // void init(const VkRenderPass &renderPass);

  void bind(VkCommandBuffer &commandBuffer, size_t currentFrame);

protected:
  virtual void initDescriptorSetLayout();
  virtual void initDescriptorPool();
  virtual void initDescriptorSets();
  virtual void initPipeline(const VkExtent2D &swapChainExtent, const VkRenderPass &renderPass,
                            std::string vertexShaderPath, std::string fragmentShaderPath);
  virtual VkShaderModule createShaderModule(const std::vector<char> &code);
};
