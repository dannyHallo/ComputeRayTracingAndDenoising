#pragma once

#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "utils/Vulkan.hpp"

#include <memory>
#include <vector>

// A template struct to hold a descriptor and the shader stage flags.
// template <typename T> struct Descriptor {
//   std::shared_ptr<T> data;
//   VkShaderStageFlags shaderStageFlags;
// };

class Logger;
class Material {
protected:
  std::vector<BufferBundle *> mUniformBufferBundles; // buffer bundles for uniform data
  std::vector<BufferBundle *> mStorageBufferBundles; // buffer bundles for storage data
  std::vector<Image *> mStorageImages;               // images for storage data

  std::string mVertexShaderPath;
  std::string mFragmentShaderPath;

  uint32_t mSwapchainSize;
  VkShaderStageFlags mShaderStageFlags;

  VkPipeline mPipeline                       = VK_NULL_HANDLE;
  VkPipelineLayout mPipelineLayout           = VK_NULL_HANDLE;
  VkDescriptorPool mDescriptorPool           = VK_NULL_HANDLE;
  VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;

  std::vector<VkDescriptorSet> mDescriptorSets;

public:
  Material(VkShaderStageFlags shaderStageFlags)
      : mSwapchainSize(
            static_cast<uint32_t>(VulkanApplicationContext::getInstance()->getSwapchainSize())),
        mShaderStageFlags(shaderStageFlags) {}

  virtual ~Material();

  // disable copy and move
  Material(const Material &)            = delete;
  Material &operator=(const Material &) = delete;
  Material(Material &&)                 = delete;
  Material &operator=(Material &&)      = delete;

  virtual void addStorageImage(Image *storageImage);
  virtual void addUniformBufferBundle(BufferBundle *uniformBufferBundle);
  virtual void addStorageBufferBundle(BufferBundle *storageBufferBundle);

  // binding should be done in model, must be overridden
  virtual void bind(VkCommandBuffer commandBuffer, size_t currentFrame) = 0;

protected:
  // creates mDescriptorSetLayout from resources added to the material
  void initDescriptorSetLayout();

  // allocates pool sizes and creates mDescriptorPool
  void initDescriptorPool();

  void initDescriptorSets();

  static VkShaderModule createShaderModule(const std::vector<char> &code);

  // late initialization during model creation, must be overridden
  virtual void init(Logger *logger) = 0;
};
