#pragma once

#include "volk.h"

#include "vma/vk_mem_alloc.h"

enum class MemoryStyle {
  kDedicated,
  kHostVisible,
};

// the wrapper class of VkBuffer, handles memory allocation and data filling
class Buffer {
public:
  Buffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, MemoryStyle memoryStyle);
  ~Buffer();

  // default copy and move
  Buffer(const Buffer &)            = default;
  Buffer &operator=(const Buffer &) = default;
  Buffer(Buffer &&)                 = default;
  Buffer &operator=(Buffer &&)      = default;

  // fill buffer with data
  //  buffer will be zero-initialized if data is nullptr
  void fillData(const void *data = nullptr);
  void fetchData(void *data);

  void *mapMemory();
  void unmapMemory();

  [[nodiscard]] VmaAllocation getMainBufferAllocation() const { return _bufferAllocation; }

  inline VkBuffer &getVkBuffer() { return _vkBuffer; }

  inline VmaAllocation &getAllocation() { return _bufferAllocation; }

  [[nodiscard]] inline VkDeviceSize getSize() const { return _size; }

  VkBufferMemoryBarrier getMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

  inline VkDescriptorBufferInfo getDescriptorInfo() {
    VkDescriptorBufferInfo descriptorInfo{};
    descriptorInfo.buffer = _vkBuffer;
    descriptorInfo.offset = 0;
    descriptorInfo.range  = _size;
    return descriptorInfo;
  }

private:
  VkDeviceSize _size; // total size of buffer

  MemoryStyle _memoryStyle;

  VkBuffer _vkBuffer              = VK_NULL_HANDLE;
  VmaAllocation _bufferAllocation = VK_NULL_HANDLE;
  void *_mappedAddr               = nullptr;

  void _allocate(VkBufferUsageFlags bufferUsageFlags);

  struct StagingBufferHandle {
    VkBuffer vkBuffer              = VK_NULL_HANDLE;
    VmaAllocation bufferAllocation = VK_NULL_HANDLE;
    void *mappedAddr               = nullptr;
  };

  // create a staging buffer that is both allowed to be transferred to and from
  [[nodiscard]] StagingBufferHandle _createStagingBuffer() const;
  static void _destroyStagingBuffer(StagingBufferHandle &stagingBufferHandle);
};
