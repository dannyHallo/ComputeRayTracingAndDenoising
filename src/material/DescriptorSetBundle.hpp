#pragma once

#include "memory/Image.hpp"

#include "utils/incl/Vulkan.hpp"

#include <unordered_set>
#include <utility>
#include <vector>

class Buffer;
class BufferBundle;
class VulkanApplicationContext;
class Logger;
// for easy management of descriptor sets, auto resource management
class DescriptorSetBundle {
public:
  DescriptorSetBundle(VulkanApplicationContext *appContext, Logger *logger, size_t bundleSize,
                      VkShaderStageFlags shaderStageFlags)
      : _appContext(appContext), _logger(logger), _bundleSize(bundleSize),
        _shaderStageFlags(shaderStageFlags) {}
  ~DescriptorSetBundle();

  // default copy and move
  DescriptorSetBundle(const DescriptorSetBundle &)            = default;
  DescriptorSetBundle &operator=(const DescriptorSetBundle &) = default;
  DescriptorSetBundle(DescriptorSetBundle &&)                 = default;
  DescriptorSetBundle &operator=(DescriptorSetBundle &&)      = default;

  [[nodiscard]] size_t getBundleSize() const { return _bundleSize; }
  [[nodiscard]] VkDescriptorSet &getDescriptorSet(size_t index) { return _descriptorSets[index]; }
  [[nodiscard]] VkDescriptorSetLayout &getDescriptorSetLayout() { return _descriptorSetLayout; }

  void bindUniformBufferBundle(uint32_t bindingNo, BufferBundle *bufferBundle);
  void bindStorageImage(uint32_t bindingNo, Image *storageImage);
  void bindStorageBuffer(uint32_t bindingNo, Buffer *buffer);

  void create();

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  size_t _bundleSize;
  VkShaderStageFlags _shaderStageFlags;

  // the pool is created per descriptor set bundle
  VkDescriptorPool _descriptorPool           = VK_NULL_HANDLE;
  VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

  std::unordered_set<uint32_t> _boundedSlots{}; // used to check for duplicated bindings
  std::vector<std::pair<uint32_t, BufferBundle *>> _uniformBufferBundles{};
  std::vector<std::pair<uint32_t, Image *>> _storageImages{};
  std::vector<std::pair<uint32_t, Buffer *>> _storageBuffers{};

  std::vector<VkDescriptorSet> _descriptorSets{};

  void _createDescriptorPool();
  void _createDescriptorSetLayout();
  void _createDescriptorSets();
  void _createDescriptorSet(uint32_t descriptorSetIndex);
};