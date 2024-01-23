#include "Buffer.hpp"
#include "utils/logger/Logger.hpp"

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags,
               const void *data)
    : _size(size) {
  _allocate(size, usage, flags);
  if (data != nullptr) {
    fillData(data);
  }
}

Buffer::~Buffer() {
  if (_vkBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(VulkanApplicationContext::getInstance()->getAllocator(), _vkBuffer,
                     _allocation);
    _vkBuffer = VK_NULL_HANDLE;
  }
}

void Buffer::_allocate(VkDeviceSize size, VkBufferUsageFlags usage,
                       VmaAllocationCreateFlags flags) {
  _size = size;

  VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // exclusive for one queue family only

  VmaAllocationCreateInfo vmaallocInfo{};
  // VMA_MEMORY_USAGE_AUTO can be used here because VkBufferCreateInfo is provided
  vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  vmaallocInfo.flags = flags;

  vmaCreateBuffer(VulkanApplicationContext::getInstance()->getAllocator(), &bufferInfo,
                  &vmaallocInfo, &_vkBuffer, &_allocation, nullptr);
}

void Buffer::fillData(const void *data) {
  // a pointer to the first byte of the allocated memory
  void *mappedData = nullptr;
  vmaMapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _allocation, &mappedData);
  memcpy(mappedData, data, _size);
  vmaUnmapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _allocation);
}
