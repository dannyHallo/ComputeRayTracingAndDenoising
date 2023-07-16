#include "Buffer.h"
#include "utils/logger.h"

Buffer::~Buffer() {
  if (mVkBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vulkanApplicationContext.getAllocator(), mVkBuffer, mAllocation);
    mVkBuffer = VK_NULL_HANDLE;
  }
}

void Buffer::allocate(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
  mSize = size;

  VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // used in only one queue family

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage                   = memoryUsage;

  VkResult result = vmaCreateBuffer(vulkanApplicationContext.getAllocator(), &bufferInfo, &vmaallocInfo, &mVkBuffer,
                                    &mAllocation, nullptr);
  logger::checkStep("vmaCreateBuffer", result);
}

void Buffer::fillData(const void *data) {
  // a pointer to the first byte of the allocated memory
  void *mappedData;
  vmaMapMemory(vulkanApplicationContext.getAllocator(), mAllocation, &mappedData);

  if (data != nullptr)
    memcpy(mappedData, data, mSize);
  else
    memset(mappedData, 0, mSize);

  vmaUnmapMemory(vulkanApplicationContext.getAllocator(), mAllocation);
}

std::shared_ptr<Buffer> BufferBundle::getBuffer(size_t index) {
  if (index < 0 || index >= mBuffers.size()) {
    logger::throwError("BufferBundle::getBuffer: index out of range");
    return nullptr; // (unreachable code)
  }
  return mBuffers[index];
}

void BufferBundle::allocate(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
  for (auto &buffer : mBuffers) buffer->allocate(size, usage, memoryUsage);
}

void BufferBundle::fillData(const void *data) {
  for (auto &buffer : mBuffers) buffer->fillData(data);
}