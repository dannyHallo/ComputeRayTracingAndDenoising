#include "Buffer.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "utils/vulkan/SimpleCommands.hpp"

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

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, MemoryStyle memoryStyle)
    : _size(size), _memoryStyle(memoryStyle) {
  _allocate(bufferUsageFlags);
}

Buffer::~Buffer() {
  if (_mainVkBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(VulkanApplicationContext::getInstance()->getAllocator(), _mainVkBuffer,
                     _mainBufferAllocation);
    _mainVkBuffer = VK_NULL_HANDLE;
  }
}

void Buffer::_allocate(VkBufferUsageFlags bufferUsageFlags) {
  VmaAllocator allocator = VulkanApplicationContext::getInstance()->getAllocator();

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
  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &_mainVkBuffer,
                  &_mainBufferAllocation, &allocInfo);

  if (_memoryStyle == MemoryStyle::kHostVisible) {
    _mainBufferMappedAddr = allocInfo.pMappedData;
  }
}

VkBufferMemoryBarrier Buffer::getMemoryBarrier(VkAccessFlags srcAccessMask,
                                               VkAccessFlags dstAccessMask) {
  VkBufferMemoryBarrier memoryBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
  memoryBarrier.srcAccessMask = srcAccessMask;
  memoryBarrier.dstAccessMask = dstAccessMask;
  memoryBarrier.buffer        = _mainVkBuffer;
  memoryBarrier.size          = _size;
  memoryBarrier.offset        = 0;
  memoryBarrier.srcQueueFamilyIndex =
      VulkanApplicationContext::getInstance()->getGraphicsQueueIndex();
  memoryBarrier.dstQueueFamilyIndex =
      VulkanApplicationContext::getInstance()->getGraphicsQueueIndex();
  return memoryBarrier;
}

Buffer::StagingBufferHandle Buffer::_createStagingBuffer() const {
  StagingBufferHandle stagingBufferHandle{};

  VmaAllocator allocator = VulkanApplicationContext::getInstance()->getAllocator();

  VkBufferCreateInfo stagingBufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stagingBufCreateInfo.size               = _size;
  stagingBufCreateInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

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
  vmaDestroyBuffer(VulkanApplicationContext::getInstance()->getAllocator(),
                   stagingBufferHandle.vkBuffer, stagingBufferHandle.bufferAllocation);
  stagingBufferHandle.vkBuffer         = VK_NULL_HANDLE;
  stagingBufferHandle.bufferAllocation = VK_NULL_HANDLE;
  stagingBufferHandle.mappedAddr       = nullptr;
}

void Buffer::fillData(const void *data) {
  auto const &device      = VulkanApplicationContext::getInstance()->getDevice();
  auto const &queue       = VulkanApplicationContext::getInstance()->getGraphicsQueue();
  auto const &commandPool = VulkanApplicationContext::getInstance()->getCommandPool();

  switch (_memoryStyle) {
  case MemoryStyle::kHostVisible: {
    assert(_mainBufferMappedAddr != nullptr && "buffer is not host visible");
    memcpy(_mainBufferMappedAddr, data, _size);
    return;
  }
  case MemoryStyle::kDedicated: {
    StagingBufferHandle stagingBufferHandle = _createStagingBuffer();

    // fill data to staging
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    memcpy(stagingBufferHandle.mappedAddr, data, _size);

    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_HOST_BIT,     // source stage
                         VK_PIPELINE_STAGE_TRANSFER_BIT, // destination stage
                         0,                              // dependency flags
                         1,                              // memory barrier count
                         &memoryBarrier,                 // memory barriers
                         0,                              // buffer memory barrier count
                         nullptr,                        // buffer memory barriers
                         0,                              // image memory barrier count
                         nullptr                         // image memory barriers
    );

    VkBufferCopy bufCopy = {
        0,     // srcOffset
        0,     // dstOffset,
        _size, // size
    };

    // copy staging to main buffer
    vkCmdCopyBuffer(commandBuffer, stagingBufferHandle.vkBuffer, _mainVkBuffer, 1, &bufCopy);

    VkMemoryBarrier memoryBarrier2{};
    memoryBarrier2.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,        // source stage
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
                         0,                                     // dependency flags
                         1,                                     // memory barrier count
                         &memoryBarrier2,                       // memory barriers
                         0,                                     // buffer memory barrier count
                         nullptr,                               // buffer memory barriers
                         0,                                     // image memory barrier count
                         nullptr                                // image memory barriers
    );

    endSingleTimeCommands(device, commandPool, queue, commandBuffer);

    _destroyStagingBuffer(stagingBufferHandle);
  }
  }

  // // map memory, copy data, unmap memory
  // void *mappedData = nullptr;
  // vmaMapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _mainBufferAllocation,
  //              &mappedData);
  // memcpy(mappedData, data, _size);
  // vmaUnmapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _mainBufferAllocation);
}
