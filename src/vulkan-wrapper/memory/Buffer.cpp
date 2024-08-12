#include "Buffer.hpp"

#include "../utils/SimpleCommands.hpp"
#include "app-context/VulkanApplicationContext.hpp"

#include <cassert>

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
namespace {
VmaAllocationCreateFlags _decideAllocationCreateFlags(MemoryStyle memoryAccessingStyle) {
  VmaAllocationCreateFlags allocFlags = 0;
  switch (memoryAccessingStyle) {
  case MemoryStyle::kDedicated:
    allocFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    break;
  case MemoryStyle::kHostVisible:
    // memory is allocated with a fixed mapped address, and host can write to it sequentially
    allocFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    break;
  }
  return allocFlags;
}
} // namespace

Buffer::Buffer(VulkanApplicationContext *appContext, VkDeviceSize size,
               VkBufferUsageFlags bufferUsageFlags, MemoryStyle memoryStyle)
    : _appContext(appContext), _size(size), _memoryStyle(memoryStyle) {
  _allocate(bufferUsageFlags);
}

Buffer::~Buffer() {
  if (_vkBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(_appContext->getAllocator(), _vkBuffer, _bufferAllocation);
    _vkBuffer = VK_NULL_HANDLE;
  }
}

void Buffer::_allocate(VkBufferUsageFlags bufferUsageFlags) {
  VmaAllocator allocator = _appContext->getAllocator();

  // all dedicated allocations are allowed to be trandferred on both directions, jfor simplicity
  if (_memoryStyle == MemoryStyle::kDedicated) {
    bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  auto vmaAlloationCreateFlags = _decideAllocationCreateFlags(_memoryStyle);

  VkBufferCreateInfo bufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size  = _size;
  bufferCreateInfo.usage = bufferUsageFlags;

  VmaAllocationCreateInfo allocCreateInfo{};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags = vmaAlloationCreateFlags;

  VmaAllocationInfo allocInfo{};
  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &_vkBuffer, &_bufferAllocation,
                  &allocInfo);

  if (_memoryStyle == MemoryStyle::kHostVisible) {
    _mappedAddr = allocInfo.pMappedData;
  }
}

VkBufferMemoryBarrier Buffer::getMemoryBarrier(VkAccessFlags srcAccessMask,
                                               VkAccessFlags dstAccessMask) {
  VkBufferMemoryBarrier memoryBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
  memoryBarrier.srcAccessMask       = srcAccessMask;
  memoryBarrier.dstAccessMask       = dstAccessMask;
  memoryBarrier.buffer              = _vkBuffer;
  memoryBarrier.size                = _size;
  memoryBarrier.offset              = 0;
  memoryBarrier.srcQueueFamilyIndex = _appContext->getGraphicsQueueIndex();
  memoryBarrier.dstQueueFamilyIndex = _appContext->getGraphicsQueueIndex();
  return memoryBarrier;
}

Buffer::StagingBufferHandle Buffer::_createStagingBuffer() const {
  StagingBufferHandle stagingBufferHandle{};

  VmaAllocator allocator = _appContext->getAllocator();

  VkBufferCreateInfo stagingBufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stagingBufCreateInfo.size               = _size;
  stagingBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VmaAllocationCreateInfo stagingAllocCreateInfo = {};
  stagingAllocCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
  stagingAllocCreateInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocationInfo stagingAllocInfo;
  vmaCreateBuffer(allocator, &stagingBufCreateInfo, &stagingAllocCreateInfo,
                  &stagingBufferHandle.vkBuffer, &stagingBufferHandle.bufferAllocation,
                  &stagingAllocInfo);
  stagingBufferHandle.mappedAddr = stagingAllocInfo.pMappedData;

  return stagingBufferHandle;
}

void Buffer::_destroyStagingBuffer(StagingBufferHandle &stagingBufferHandle) {
  vmaDestroyBuffer(_appContext->getAllocator(), stagingBufferHandle.vkBuffer,
                   stagingBufferHandle.bufferAllocation);
  stagingBufferHandle.vkBuffer         = VK_NULL_HANDLE;
  stagingBufferHandle.bufferAllocation = VK_NULL_HANDLE;
  stagingBufferHandle.mappedAddr       = nullptr;
}

void Buffer::fillData(const void *data) {
  auto const &device      = _appContext->getDevice();
  auto const &queue       = _appContext->getGraphicsQueue();
  auto const &commandPool = _appContext->getCommandPool();

  switch (_memoryStyle) {
  case MemoryStyle::kHostVisible: {
    memcpy(_mappedAddr, data, _size);
    break;
  }
  case MemoryStyle::kDedicated: {
    StagingBufferHandle stagingBufferHandle = _createStagingBuffer();
    memcpy(stagingBufferHandle.mappedAddr, data, _size);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferCopy bufCopy = {
        0,     // srcOffset
        0,     // dstOffset,
        _size, // size
    };

    // copy staging buffer to main buffer
    vkCmdCopyBuffer(commandBuffer, stagingBufferHandle.vkBuffer, _vkBuffer, 1, &bufCopy);

    endSingleTimeCommands(device, commandPool, queue, commandBuffer);
    _destroyStagingBuffer(stagingBufferHandle);
    break;
  }
  }
}

void Buffer::fetchData(void *data) {
  auto const &device      = _appContext->getDevice();
  auto const &queue       = _appContext->getGraphicsQueue();
  auto const &commandPool = _appContext->getCommandPool();

  switch (_memoryStyle) {
  case MemoryStyle::kHostVisible: {
    assert(_mappedAddr != nullptr && "_mappedAddr is nullptr");
    memcpy(data, _mappedAddr, _size);
    return;
  }

  case MemoryStyle::kDedicated: {
    StagingBufferHandle stagingBufferHandle = _createStagingBuffer();

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferCopy bufCopy = {
        0,     // srcOffset
        0,     // dstOffset,
        _size, // size
    };

    // copy main buffer to staging buffer
    vkCmdCopyBuffer(commandBuffer, _vkBuffer, stagingBufferHandle.vkBuffer, 1, &bufCopy);
    endSingleTimeCommands(device, commandPool, queue, commandBuffer);

    memcpy(data, stagingBufferHandle.mappedAddr, _size);

    _destroyStagingBuffer(stagingBufferHandle);
    break;
  }
  }
}