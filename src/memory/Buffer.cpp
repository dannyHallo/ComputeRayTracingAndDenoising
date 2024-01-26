#include "Buffer.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/vulkan/SimpleCommands.hpp"

#include <cassert>
#include <unordered_map>

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html

namespace {
const std::unordered_map<BufferType, VkBufferUsageFlags> kBufferTypeToVkBufferUsageFlags = {
    {BufferType::kUniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
    {BufferType::kStorage, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
};

void _decideFlags(BufferType bufferType, MemoryAccessingStyle memoryAccessingStyle,
                  VkBufferUsageFlags &usageFlags, VmaAllocationCreateFlags &allocFlags) {
  usageFlags = kBufferTypeToVkBufferUsageFlags.at(bufferType);
  // this kind of buffer should be optimized to be copied by staging buffer
  if (memoryAccessingStyle == MemoryAccessingStyle::kCpuToGpuOnce) {
    usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  allocFlags = 0;
  switch (memoryAccessingStyle) {
  case MemoryAccessingStyle::kGpuOnly:
  case MemoryAccessingStyle::kCpuToGpuOnce:
    allocFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    break;
  case MemoryAccessingStyle::kCpuToGpuEveryFrame:
    allocFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    break;
  }
}
} // namespace

Buffer::Buffer(VkDeviceSize size, BufferType bufferType, MemoryAccessingStyle memoryAccessingStyle)
    : _size(size) {
  _allocate(bufferType, memoryAccessingStyle);
}

Buffer::~Buffer() {
  if (_mainVkBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(VulkanApplicationContext::getInstance()->getAllocator(), _mainVkBuffer,
                     _mainBufferAllocation);
    _mainVkBuffer = VK_NULL_HANDLE;
  }

  if (_stagingVkBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(VulkanApplicationContext::getInstance()->getAllocator(), _stagingVkBuffer,
                     _stagingBufferAllocation);
    _stagingVkBuffer = VK_NULL_HANDLE;
  }
}

void Buffer::_allocate(BufferType bufferType, MemoryAccessingStyle memoryAccessingStyle) {
  if (bufferType == BufferType::kUniform) {
    assert(memoryAccessingStyle == MemoryAccessingStyle::kCpuToGpuEveryFrame &&
           "uniform buffer should only be written by CPU every frame");
  }

  VmaAllocator allocator = VulkanApplicationContext::getInstance()->getAllocator();

  VkBufferUsageFlags usageFlags       = 0;
  VmaAllocationCreateFlags allocFlags = 0;
  _decideFlags(bufferType, memoryAccessingStyle, usageFlags, allocFlags);

  VkBufferCreateInfo bufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size  = _size;
  bufferCreateInfo.usage = usageFlags;

  VmaAllocationCreateInfo allocCreateInfo{};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags = allocFlags;

  VmaAllocationInfo allocInfo{};
  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &_mainVkBuffer,
                  &_mainBufferAllocation, &allocInfo);

  if ((allocFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0U) {
    _mainBufferMappedAddr = allocInfo.pMappedData;
  }

  if (memoryAccessingStyle == MemoryAccessingStyle::kCpuToGpuOnce) {
    // Allocation ended up in a non-mappable memory - need to transfer.
    VkBufferCreateInfo stagingBufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    stagingBufCreateInfo.size               = _size;
    stagingBufCreateInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocCreateInfo = {};
    stagingAllocCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
    stagingAllocCreateInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingAllocInfo;
    vmaCreateBuffer(allocator, &stagingBufCreateInfo, &stagingAllocCreateInfo, &_stagingVkBuffer,
                    &_stagingBufferAllocation, &stagingAllocInfo);
    _stagingBufferMappedAddr = stagingAllocInfo.pMappedData;
  }
}

void Buffer::fillData(const void *data) {
  auto const &device      = VulkanApplicationContext::getInstance()->getDevice();
  auto const &queue       = VulkanApplicationContext::getInstance()->getGraphicsQueue();
  auto const &commandPool = VulkanApplicationContext::getInstance()->getCommandPool();

  // fixed pointer to the mapped memory
  if (_mainBufferMappedAddr != nullptr) {
    memcpy(_mainBufferMappedAddr, data, _size);
    return;
  }

  if (_stagingBufferMappedAddr != nullptr) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    memcpy(_stagingBufferMappedAddr, data, _size);
    // vkCmdPipelineBarrier: VK_ACCESS_HOST_WRITE_BIT --> VK_ACCESS_TRANSFER_READ_BIT
    VkBufferCopy bufCopy = {
        0,     // srcOffset
        0,     // dstOffset,
        _size, // size
    };

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

    // record a command to copy the buffer to the image
    vkCmdCopyBuffer(commandBuffer, _stagingVkBuffer, _mainVkBuffer, 1, &bufCopy);

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

    return;
  }

  // map memory, copy data, unmap memory
  void *mappedData = nullptr;
  vmaMapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _mainBufferAllocation,
               &mappedData);
  memcpy(mappedData, data, _size);
  vmaUnmapMemory(VulkanApplicationContext::getInstance()->getAllocator(), _mainBufferAllocation);
}
