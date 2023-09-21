#pragma once

#include "app-context/VulkanApplicationContext.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// the wrapper class of VkBuffer, handles memory allocation and data filling
class Buffer {
  VkBuffer mVkBuffer;        // native vulkan buffer
  VmaAllocation mAllocation; // buffer allocation info
  VkDeviceSize mSize;        // total size of buffer

public:
  /// @brief the wrapper class of VkBuffer, with easier memory allocation and
  /// data filling
  /// @param size size of buffer
  /// @param usage usage of buffer
  /// @param memoryUsage memory usage of buffer
  /// @param data data to be filled in buffer, if left as nullptr, buffer will
  /// be zero-initialized
  Buffer(VkDeviceSize size, VkBufferUsageFlags usage,
         VmaMemoryUsage memoryUsage, const void *data = nullptr);

  ~Buffer();

  // fill buffer with data, if data is nullptr, buffer will be zero-initialized
  // data size must be equal to buffer size
  void fillData(const void *data = nullptr);

  inline VkBuffer &getVkBuffer() { return mVkBuffer; }

  inline VmaAllocation &getAllocation() { return mAllocation; }

  inline const VkDeviceSize getSize() const { return mSize; }

  inline VkDescriptorBufferInfo getDescriptorInfo() {
    VkDescriptorBufferInfo descriptorInfo{};
    descriptorInfo.buffer = mVkBuffer;
    descriptorInfo.offset = 0;
    descriptorInfo.range  = mSize;
    return descriptorInfo;
  }

private:
  // buffer allocation is only allowed during construction
  void allocate(VkDeviceSize size, VkBufferUsageFlags usage,
                VmaMemoryUsage memoryUsage);
};

// class of series of buffers, can be used for storing identical buffers for
// every swapchain image
class BufferBundle {
  std::vector<std::shared_ptr<Buffer>> mBuffers; // series of buffers

public:
  /// @brief class of series of buffers, usually used for storing identical
  /// buffers for every swapchain image
  /// @param bufferCount number of buffers in bundle
  /// @param size size of every single buffer
  /// @param usage usage of every single buffer
  /// @param memoryUsage memory usage of every single buffer
  /// @param data data to be filled in every single buffer, if left as nullptr,
  /// every buffer will be zero-initialized
  BufferBundle(size_t bufferCount, VkDeviceSize size, VkBufferUsageFlags usage,
               VmaMemoryUsage memoryUsage, const void *data = nullptr) {
    for (size_t i = 0; i < bufferCount; i++) {
      mBuffers.emplace_back(
          std::make_shared<Buffer>(size, usage, memoryUsage, data));
    }
  }

  ~BufferBundle() { mBuffers.clear(); }

  // get buffer by index
  std::shared_ptr<Buffer> getBuffer(size_t index);

  // fill every buffer in bundle with the same data, if left as nullptr, every
  // buffer will be zero-initialized
  void fillData(const void *data = nullptr);
};
