#pragma once

#include "utils/incl/Vulkan.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class Logger;
class Image;
class BufferBundle;
class VulkanApplicationContext;
class DescriptorSetBundle;
class Pipeline {
public:
  Pipeline(VulkanApplicationContext *appContext, Logger *logger, std::vector<char> &&shaderCode,
           DescriptorSetBundle *descriptorSetBundle, VkShaderStageFlags shaderStageFlags)
      : _appContext(appContext), _logger(logger), _descriptorSetBundle(descriptorSetBundle),
        _shaderCode(std::move(shaderCode)), _shaderStageFlags(shaderStageFlags) {}

  virtual ~Pipeline();

  // disable copy and move
  Pipeline(const Pipeline &)            = delete;
  Pipeline &operator=(const Pipeline &) = delete;
  Pipeline(Pipeline &&)                 = delete;
  Pipeline &operator=(Pipeline &&)      = delete;

  virtual void init() = 0;

protected:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  DescriptorSetBundle *_descriptorSetBundle;
  std::vector<char> _shaderCode;

  std::vector<BufferBundle *> _uniformBufferBundles; // buffer bundles for uniform data
  std::vector<BufferBundle *> _storageBufferBundles; // buffer bundles for storage data
  std::vector<Image *> _storageImages;               // images for storage data

  VkShaderStageFlags _shaderStageFlags;

  VkPipeline _pipeline             = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

  VkShaderModule _createShaderModule(const std::vector<char> &code);
  void _bind(VkCommandBuffer commandBuffer, size_t currentFrame);
};
