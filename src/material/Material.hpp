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
  Material(VulkanApplicationContext *appContext, Logger *logger,
           VkShaderStageFlags shaderStageFlags);
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
  void init(size_t framesInFlight);

  void bind(VkCommandBuffer commandBuffer, size_t currentFrame);

protected:
  VulkanApplicationContext *_appContext = nullptr;
  Logger *_logger                       = nullptr;

  std::vector<BufferBundle *> _uniformBufferBundles; // buffer bundles for uniform data
  std::vector<BufferBundle *> _storageBufferBundles; // buffer bundles for storage data
  std::vector<Image *> _storageImages;               // images for storage data

  std::string _vertexShaderPath;
  std::string _fragmentShaderPath;

  VkShaderStageFlags _shaderStageFlags;
  VkPipelineBindPoint _pipelineBindPoint; // deducted from _shaderStageFlags

  VkPipeline _pipeline                       = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout           = VK_NULL_HANDLE;
  VkDescriptorPool _descriptorPool           = VK_NULL_HANDLE;
  VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

  std::vector<VkDescriptorSet> _descriptorSets;

  void _createDescriptorSetLayout();

  // allocates pool sizes and creates mDescriptorPool
  void _createDescriptorPool(size_t framesInFlight);

  void _createDescriptorSets(size_t framesInFlight);

  virtual void _createPipeline() = 0;

  VkShaderModule _createShaderModule(const std::vector<char> &code);

  void _bind(VkPipelineBindPoint pipelineBindPoint, VkCommandBuffer commandBuffer,
             size_t currentFrame);
};
