#pragma once

#include "app-context/VulkanApplicationContext.h"
#include "scene/Mesh.h"
#include "utils/vulkan.h"
#include "vk_mem_alloc.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// the wrapper class of VkBuffer, with easier memory allocation and data filling
class Buffer {
  VkBuffer mVkBuffer;        // native vulkan buffer
  VmaAllocation mAllocation; // buffer allocation info
  VkDeviceSize mSize;        // total size of buffer

public:
  Buffer() : mVkBuffer(VK_NULL_HANDLE), mAllocation(VK_NULL_HANDLE), mSize(0) {}
  ~Buffer();

  // allocate buffer
  void allocate(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

  // fill buffer with data, if data is nullptr, buffer will be zero-initialized
  // data size must be equal to buffer size
  void fillData(const void *data = nullptr);

  inline VkBuffer &getVkBuffer() { return mVkBuffer; }
  inline VmaAllocation &getAllocation() { return mAllocation; }
  inline VkDeviceSize getSize() const { return mSize; }

  inline VkDescriptorBufferInfo getDescriptorInfo() {
    VkDescriptorBufferInfo descriptorInfo{};
    descriptorInfo.buffer = mVkBuffer;
    descriptorInfo.offset = 0;
    descriptorInfo.range  = mSize;
    return descriptorInfo;
  }
};

// series of buffers, usually used for storing identical buffers for every swapchain image
class BufferBundle {
  std::vector<std::shared_ptr<Buffer>> mBuffers; // series of buffers

public:
  BufferBundle(size_t bufferCount) {
    for (size_t i = 0; i < bufferCount; i++) {
      mBuffers.emplace_back(std::make_shared<Buffer>());
    }
  }

  ~BufferBundle() { mBuffers.clear(); }

  // get buffer by index
  std::shared_ptr<Buffer> getBuffer(size_t index);

  // allocate every buffer in bundle
  void allocate(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

  // fill every buffer in bundle with data, if data is nullptr, every buffer will be zero-initialized
  void fillData(const void *data = nullptr);
};
