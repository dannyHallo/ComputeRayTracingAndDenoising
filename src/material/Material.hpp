#pragma once

#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "utils/incl/Vulkan.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class Logger;
class VulkanApplicationContext;
class DescriptorSetBundle;
class Material {
protected:
public:
  Material(VulkanApplicationContext *appContext, Logger *logger, std::vector<uint32_t> &&shaderCode,
           DescriptorSetBundle *descriptorSetBundle, VkShaderStageFlags shaderStageFlags)
      : _appContext(appContext), _logger(logger), _descriptorSetBundle(descriptorSetBundle),
        _shaderCode(std::move(shaderCode)), _shaderStageFlags(shaderStageFlags) {}

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
  void init();

  void bind(VkCommandBuffer commandBuffer, size_t currentFrame);

protected:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  DescriptorSetBundle *_descriptorSetBundle;
  std::vector<uint32_t> _shaderCode;

  std::vector<BufferBundle *> _uniformBufferBundles; // buffer bundles for uniform data
  std::vector<BufferBundle *> _storageBufferBundles; // buffer bundles for storage data
  std::vector<Image *> _storageImages;               // images for storage data

  VkShaderStageFlags _shaderStageFlags;

  VkPipeline _pipeline             = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

  virtual void _createPipeline() = 0;

  VkShaderModule _createShaderModule(const std::vector<uint32_t> &code);

  void _bind(VkPipelineBindPoint pipelineBindPoint, VkCommandBuffer commandBuffer,
             size_t currentFrame);
};
