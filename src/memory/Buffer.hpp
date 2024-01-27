#pragma once

#include "app-context/VulkanApplicationContext.hpp"

enum class MemoryAccessingStyle {
  kGpuOnly,
  kCpuToGpuOnce,
  kCpuToGpuEveryFrame,
};

// the wrapper class of VkBuffer, handles memory allocation and data filling
class Buffer {
public:
  Buffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags,
         MemoryAccessingStyle memoryAccessingStyle);
  ~Buffer();

  // default copy and move
  Buffer(const Buffer &)            = default;
  Buffer &operator=(const Buffer &) = default;
  Buffer(Buffer &&)                 = default;
  Buffer &operator=(Buffer &&)      = default;

  // fill buffer with data, if data is nullptr, buffer will be zero-initialized
  // data size must be equal to buffer size
  void fillData(const void *data = nullptr);

  inline VkBuffer &getVkBuffer() { return _mainVkBuffer; }

  inline VmaAllocation &getAllocation() { return _mainBufferAllocation; }

  [[nodiscard]] inline VkDeviceSize getSize() const { return _size; }

  inline VkDescriptorBufferInfo getDescriptorInfo() {
    VkDescriptorBufferInfo descriptorInfo{};
    descriptorInfo.buffer = _mainVkBuffer;
    descriptorInfo.offset = 0;
    descriptorInfo.range  = _size;
    return descriptorInfo;
  }

private:
  VkDeviceSize _size; // total size of buffer

  VkBuffer _mainVkBuffer              = VK_NULL_HANDLE;
  VmaAllocation _mainBufferAllocation = VK_NULL_HANDLE;
  void *_mainBufferMappedAddr         = nullptr;

  VkBuffer _stagingVkBuffer              = VK_NULL_HANDLE;
  VmaAllocation _stagingBufferAllocation = VK_NULL_HANDLE;
  void *_stagingBufferMappedAddr         = nullptr;

  // buffer allocation is only allowed during construction
  void _allocate(VkBufferUsageFlags bufferUsageFlagse, MemoryAccessingStyle memoryAccessingStyle);
};
