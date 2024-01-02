#pragma once

#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "utils/incl/Vulkan.hpp"

#include <memory>
#include <vector>

class Logger;
class VulkanApplicationContext;
class Material {
protected:
public:
  Material(VulkanApplicationContext *appContext, VkShaderStageFlags shaderStageFlags)
      : _appContext(appContext), _shaderStageFlags(shaderStageFlags) {}

  virtual ~Material();

  // disable copy and move
  Material(const Material &)            = delete;
  Material &operator=(const Material &) = delete;
  Material(Material &&)                 = delete;
  Material &operator=(Material &&)      = delete;

  void addStorageImage(Image *storageImage);
  void addUniformBufferBundle(BufferBundle *uniformBufferBundle);
  void addStorageBufferBundle(BufferBundle *storageBufferBundle);

  // late initialization during model creation, must be overridden
  virtual void init(Logger *logger) = 0;

  // binding should be done in model, must be overridden
  void bind(VkCommandBuffer commandBuffer, size_t currentFrame);

protected:
  VulkanApplicationContext *_appContext = nullptr;

  std::vector<BufferBundle *> _uniformBufferBundles; // buffer bundles for uniform data
  std::vector<BufferBundle *> _storageBufferBundles; // buffer bundles for storage data
  std::vector<Image *> _storageImages;               // images for storage data

  std::string _vertexShaderPath;
  std::string _fragmentShaderPath;

  VkShaderStageFlags _shaderStageFlags;

  VkPipeline _pipeline                       = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout           = VK_NULL_HANDLE;
  VkDescriptorPool _descriptorPool           = VK_NULL_HANDLE;
  VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

  std::vector<VkDescriptorSet> _descriptorSets;

  void _createDescriptorSetLayout();

  // allocates pool sizes and creates mDescriptorPool
  void _createDescriptorPool();

  void _createDescriptorSets();

  VkShaderModule _createShaderModule(const std::vector<char> &code);

  void _bind(VkPipelineBindPoint pipelineBindPoint, VkCommandBuffer commandBuffer,
             size_t currentFrame);
};
