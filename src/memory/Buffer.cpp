#include "Buffer.hpp"
#include "utils/Logger.hpp"

#include <cassert>

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
               const void *data)
    : _vkBuffer(VK_NULL_HANDLE), _allocation(VK_NULL_HANDLE), _size(size) {
  _allocate(size, usage, memoryUsage);
  fillData(data);
}

Buffer::~Buffer() {
  if (_vkBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(VulkanApplicationContext::getInstance()->getAllocator(), _vkBuffer,
                     _allocation);
    _vkBuffer = VK_NULL_HANDLE;
  }
}

void Buffer::_allocate(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
  _size = size;

  VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // used in only one queue family

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage                   = memoryUsage;

  VkResult result = vmaCreateBuffer(VulkanApplicationContext::getInstance()->getAllocator(),
                                    &bufferInfo, &vmaallocInfo, &_vkBuffer, &_allocation, nullptr);
  assert(result == VK_SUCCESS && "Buffer::allocate: failed to allocate buffer");
}

void Buffer::fillData(const void *data) {
  // a pointer to the first byte of the allocated memory
  void *mappedData;
  vmaMapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _allocation, &mappedData);

  if (data != nullptr)
    memcpy(mappedData, data, _size);
  else
    memset(mappedData, 0, _size);

  vmaUnmapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _allocation);
}

std::shared_ptr<Buffer> BufferBundle::getBuffer(size_t index) {
  assert((index >= 0 && index < _buffers.size()) && "BufferBundle::getBuffer: index out of range");
  return _buffers[index];
}

void BufferBundle::fillData(const void *data) {
  for (auto &buffer : _buffers) buffer->fillData(data);
}