#pragma once

#include "utils/incl/Vulkan.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Logger;
class Image;
class BufferBundle;
class VulkanApplicationContext;
class DescriptorSetBundle;
class Pipeline {
public:
  Pipeline(VulkanApplicationContext *appContext, Logger *logger, std::string fileName,
           DescriptorSetBundle *descriptorSetBundle, VkShaderStageFlags shaderStageFlags);
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
  std::string _fileName;

  std::vector<BufferBundle *> _uniformBufferBundles; // buffer bundles for uniform data
  std::vector<BufferBundle *> _storageBufferBundles; // buffer bundles for storage data
  std::vector<Image *> _storageImages;               // images for storage data

  VkShaderStageFlags _shaderStageFlags;

  VkPipeline _pipeline             = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

  VkShaderModule _createShaderModule(const std::vector<uint32_t> &code);
  void _bind(VkCommandBuffer commandBuffer, size_t currentFrame);
};
