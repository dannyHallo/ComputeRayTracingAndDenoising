#pragma once

#include "app-context/VulkanApplicationContext.hpp"

enum class BufferType {
  kUniform,
  kStorage,
};

// the wrapper class of VkBuffer, handles memory allocation and data filling
class Buffer {
public:
  Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags,
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
  VkBuffer _vkBuffer        = VK_NULL_HANDLE; // native vulkan buffer
  VmaAllocation _allocation = VK_NULL_HANDLE; // buffer allocation info
  VkDeviceSize _size;                         // total size of buffer

  // buffer allocation is only allowed during construction
  void _allocate(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags);
};
