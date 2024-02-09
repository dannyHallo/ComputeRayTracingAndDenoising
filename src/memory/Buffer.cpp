#include "Buffer.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/vulkan/SimpleCommands.hpp"

#include <unordered_map>

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html

namespace {
VmaAllocationCreateFlags _decideAllocationCreateFlags(MemoryAccessingStyle memoryAccessingStyle) {

  VmaAllocationCreateFlags allocFlags = 0;
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
  return allocFlags;
}

} // namespace

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags,
               MemoryAccessingStyle memoryAccessingStyle)
    : _size(size) {
  _allocate(bufferUsageFlags, memoryAccessingStyle);
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

void Buffer::_allocate(VkBufferUsageFlags bufferUsageFlags,
                       MemoryAccessingStyle memoryAccessingStyle) {
  VmaAllocator allocator = VulkanApplicationContext::getInstance()->getAllocator();

  // this kind of buffer should be optimized to be copied by staging buffer
  if (memoryAccessingStyle == MemoryAccessingStyle::kCpuToGpuOnce) {
    bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  auto vmaAlloationCreateFlags = _decideAllocationCreateFlags(memoryAccessingStyle);

  VkBufferCreateInfo bufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size  = _size;
  bufferCreateInfo.usage = bufferUsageFlags;

  VmaAllocationCreateInfo allocCreateInfo{};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags = vmaAlloationCreateFlags;

  VmaAllocationInfo allocInfo{};
  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &_mainVkBuffer,
                  &_mainBufferAllocation, &allocInfo);

  if ((vmaAlloationCreateFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0U) {
    _mainBufferMappedAddr = allocInfo.pMappedData;
  }

  if (memoryAccessingStyle == MemoryAccessingStyle::kCpuToGpuOnce) {
    // allocation ended up in a non-mappable memory - need to transfer using a staging buffer
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

void Buffer::fillData(const void *data) {
  // TODO: when using kCpuToGpuOnce, destroy the staging buffer once the data is copied

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
