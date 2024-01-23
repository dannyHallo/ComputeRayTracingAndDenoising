#include "Buffer.hpp"
#include "utils/logger/Logger.hpp"

#include <unordered_map>

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html

Buffer::Buffer(VkDeviceSize size, BufferType bufferType) : _size(size) { _allocate(bufferType); }

Buffer::~Buffer() {
  if (_vkBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(VulkanApplicationContext::getInstance()->getAllocator(), _vkBuffer,
                     _allocation);
    _vkBuffer = VK_NULL_HANDLE;
  }
}

void Buffer::_allocate(BufferType bufferType) {
  switch (bufferType) {
  case BufferType::kUniform:
    _allocateUniformBuffer();
    break;
  case BufferType::kStorage:
    _allocateStorageBuffer();
    break;
  }
}

void Buffer::_allocateUniformBuffer() {
  VmaAllocator allocator = VulkanApplicationContext::getInstance()->getAllocator();

  VkBufferCreateInfo bufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size  = _size;
  bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VmaAllocationCreateInfo allocCreateInfo{};
  // VMA_MEMORY_USAGE_AUTO can be used here because VkBufferCreateInfo is provided
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags =
      // so we can use vkMapMemory and memcpy(), random access is unoptimized, use
      // VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT instead
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      //  it says that despite request for host access, a not-HOST_VISIBLE memory type can be
      //  selected if it may improve performance.
      VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
      // so memory will be persistently mapped (don't need to vmMapMemory every frame)
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocationInfo allocInfo{};
  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &_vkBuffer, &_allocation,
                  &allocInfo);

  VkMemoryPropertyFlags memPropFlags = 0;
  vmaGetAllocationMemoryProperties(allocator, _allocation, &memPropFlags);

  // allocation ended up in a mappable memory and is already mapped - write to it directly.
  if ((memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0U) {
    _mappedData = allocInfo.pMappedData;
  } else {
    assert(false && "not implemented");

    // // Allocation ended up in a non-mappable memory - need to transfer.
    // VkBufferCreateInfo stagingBufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    // stagingBufCreateInfo.size               = 65536;
    // stagingBufCreateInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    // VmaAllocationCreateInfo stagingAllocCreateInfo = {};
    // stagingAllocCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
    // stagingAllocCreateInfo.flags =
    //     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
    //     VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // VkBuffer stagingBuf;
    // VmaAllocation stagingAlloc;
    // VmaAllocationInfo stagingAllocInfo;
    // vmaCreateBuffer(allocator, &stagingBufCreateInfo, &stagingAllocCreateInfo, &stagingBuf,
    //                 &stagingAlloc, stagingAllocInfo);

    // // [Executed in runtime]:
    // memcpy(stagingAllocInfo.pMappedData, myData, myDataSize);
    // vmaFlushAllocation(allocator, stagingAlloc, 0, VK_WHOLE_SIZE);
    // // vkCmdPipelineBarrier: VK_ACCESS_HOST_WRITE_BIT --> VK_ACCESS_TRANSFER_READ_BIT
    // VkBufferCopy bufCopy = {
    //     0, // srcOffset
    //     0, // dstOffset,
    //     myDataSize); // size
    // vkCmdCopyBuffer(cmdBuf, stagingBuf, buf, 1, &bufCopy);
  }
}

void Buffer::_allocateStorageBuffer() {
  VmaAllocator allocator = VulkanApplicationContext::getInstance()->getAllocator();

  VkBufferCreateInfo bufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size  = _size;
  bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  VmaAllocationCreateInfo allocCreateInfo{};
  // VMA_MEMORY_USAGE_AUTO can be used here because VkBufferCreateInfo is provided
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  allocCreateInfo.flags =
      // so we can use vkMapMemory and memcpy(), random access is unoptimized, use
      // VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT instead
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  VmaAllocationInfo allocInfo{};
  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &_vkBuffer, &_allocation,
                  &allocInfo);
}

void Buffer::fillData(const void *data) {
  // a pointer to the first byte of the allocated memory
  void *mappedData = nullptr;
  vmaMapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _allocation, &mappedData);
  memcpy(mappedData, data, _size);
  vmaUnmapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _allocation);
}
