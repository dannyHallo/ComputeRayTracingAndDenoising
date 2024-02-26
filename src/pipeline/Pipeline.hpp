#pragma once

#include "volk/volk.h"

#include <cstdint>
#include <string>
#include <vector>

class Logger;
class Image;
class BufferBundle;
class VulkanApplicationContext;
class DescriptorSetBundle;
class Scheduler;
class ShaderChangeListener;

class Pipeline {
public:
  Pipeline(VulkanApplicationContext *appContext, Logger *logger, Scheduler *scheduler,
           std::string shaderFileName, DescriptorSetBundle *descriptorSetBundle,
           VkShaderStageFlags shaderStageFlags,
           ShaderChangeListener *shaderChangeListener = nullptr, bool needToRebuildSvo = false);
  virtual ~Pipeline();

  // disable copy and move
  Pipeline(const Pipeline &)            = delete;
  Pipeline &operator=(const Pipeline &) = delete;
  Pipeline(Pipeline &&)                 = delete;
  Pipeline &operator=(Pipeline &&)      = delete;

  virtual void build() = 0;

  // build the shader module and cache it
  // if allowBuildFail is set to true, function returns if the shader module is built successfully
  // if allowBuildFail is set to false, the entire program exits if the shader module is not built
  // successfully
  virtual bool buildAndCacheShaderModule(bool allowBuildFail) = 0;

  void updateDescriptorSetBundle(DescriptorSetBundle *descriptorSetBundle);

  [[nodiscard]] std::string getShaderFileName() const { return _shaderFileName; }
  [[nodiscard]] Scheduler *getScheduler() const { return _scheduler; }

protected:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  Scheduler *_scheduler;
  ShaderChangeListener *_shaderChangeListener;
  VkShaderModule _cachedShaderModule = VK_NULL_HANDLE;

  DescriptorSetBundle *_descriptorSetBundle;
  std::string _shaderFileName;

  std::vector<BufferBundle *> _uniformBufferBundles; // buffer bundles for uniform data
  std::vector<BufferBundle *> _storageBufferBundles; // buffer bundles for storage data
  std::vector<Image *> _storageImages;               // images for storage data

  VkShaderStageFlags _shaderStageFlags;

  VkPipeline _pipeline             = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

  void _cleanupPipelineAndLayout();
  void _cleanupShaderModule();

  VkShaderModule _createShaderModule(const std::vector<uint32_t> &code);
  void _bind(VkCommandBuffer commandBuffer, size_t currentFrame);
};
