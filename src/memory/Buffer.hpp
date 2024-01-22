#pragma once

#include "app-context/VulkanApplicationContext.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// the wrapper class of VkBuffer, handles memory allocation and data filling
class Buffer {
public:
  Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
         const void *data = nullptr);
  ~Buffer();

  // default copy and move
  Buffer(const Buffer &)            = default;
  Buffer &operator=(const Buffer &) = default;
  Buffer(Buffer &&)                 = default;
  Buffer &operator=(Buffer &&)      = default;

  // fill buffer with data, if data is nullptr, buffer will be zero-initialized
  // data size must be equal to buffer size
  void fillData(const void *data = nullptr);

  inline VkBuffer &getVkBuffer() { return _vkBuffer; }

  inline VmaAllocation &getAllocation() { return _allocation; }

  [[nodiscard]] inline VkDeviceSize getSize() const { return _size; }

  inline VkDescriptorBufferInfo getDescriptorInfo() {
    VkDescriptorBufferInfo descriptorInfo{};
    descriptorInfo.buffer = _vkBuffer;
    descriptorInfo.offset = 0;
    descriptorInfo.range  = _size;
    return descriptorInfo;
  }

private:
  VkBuffer _vkBuffer;        // native vulkan buffer
  VmaAllocation _allocation; // buffer allocation info
  VkDeviceSize _size;        // total size of buffer

  // buffer allocation is only allowed during construction
  void _allocate(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
};

// class of series of buffers, can be used for storing identical buffers for
// every swapchain image
class BufferBundle {
public:
  BufferBundle(size_t bundleSize, VkDeviceSize perBufferSize, VkBufferUsageFlags usage,
               VmaMemoryUsage memoryUsage, const void *data = nullptr) {
    _buffers.reserve(bundleSize);
    for (size_t i = 0; i < bundleSize; i++) {
      _buffers.emplace_back(std::make_unique<Buffer>(perBufferSize, usage, memoryUsage, data));
    }
  }

  ~BufferBundle() { _buffers.clear(); }

  // delete copy and move
  BufferBundle(const BufferBundle &)            = delete;
  BufferBundle &operator=(const BufferBundle &) = delete;
  BufferBundle(BufferBundle &&)                 = delete;
  BufferBundle &operator=(BufferBundle &&)      = delete;

  [[nodiscard]] size_t getBundleSize() const { return _buffers.size(); }

  Buffer *getBuffer(size_t index);
  Buffer *operator[](size_t index);

  // fill every buffer in bundle with the same data, if left as nullptr, every
  // buffer will be zero-initialized
  void fillData(const void *data = nullptr);

private:
  std::vector<std::unique_ptr<Buffer>> _buffers; // series of buffers
};
