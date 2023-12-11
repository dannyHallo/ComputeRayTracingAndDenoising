#include "Image.hpp"

#include "app-context/VulkanApplicationContext.h"

#include "Buffer.hpp"
#include "render-context/RenderSystem.hpp"
#include "utils/Logger.hpp"
#include "utils/Vulkan.hpp"

#define STB_IMAGE_IMPLEMENTATION
// disable all warnings from stb_image (gcc & clang)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include "stb-image/stb_image.h"
#pragma GCC diagnostic pop

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>

static const VkClearColorValue kClearColor = {{0, 0, 0, 0}};

static const std::unordered_map<VkFormat, int> kVkFormatBytesPerPixelMap{
    // 8-bit formats
    {VK_FORMAT_R8_UNORM, 1},
    {VK_FORMAT_R8G8_UNORM, 2},
    {VK_FORMAT_R8G8B8_UNORM, 3},
    {VK_FORMAT_R8G8B8A8_UNORM, 4},

    // 16-bit formats
    {VK_FORMAT_R16_UNORM, 2},
    {VK_FORMAT_R16G16_UNORM, 4},
    {VK_FORMAT_R16G16B16_UNORM, 6},
    {VK_FORMAT_R16G16B16A16_UNORM, 8},

    // 32-bit formats
    {VK_FORMAT_R32_SFLOAT, 4},
    {VK_FORMAT_R32G32_SFLOAT, 8},
    {VK_FORMAT_R32G32B32_SFLOAT, 12},
    {VK_FORMAT_R32G32B32A32_SFLOAT, 16},
};

namespace {

unsigned char *_loadImageFromPath(const std::string &path, int &width, int &height, int &channels) {
  unsigned char *imageData = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  assert(imageData != nullptr && "Failed to load texture image");
  return imageData;
}

void _freeImageData(unsigned char *imageData) { stbi_image_free(imageData); }

} // namespace

Image::Image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
             VkImageLayout initialImageLayout, VkSampleCountFlagBits numSamples,
             VkImageTiling tiling, VkImageAspectFlags aspectFlags, VmaMemoryUsage memoryUsage)
    : mCurrentImageLayout(VK_IMAGE_LAYOUT_UNDEFINED), mLayerCount(1), mFormat(format),
      mWidth(width), mHeight(height) {
  VkResult result = _createImage(numSamples, tiling, usage, memoryUsage);
  assert(result == VK_SUCCESS && "failed to create image!");

  if (initialImageLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
    _transitionImageLayout(initialImageLayout);
  }
  mVkImageView = createImageView(VulkanApplicationContext::getInstance()->getDevice(), mVkImage,
                                 format, aspectFlags, mLayerCount);
}

Image::Image(const std::string &filename, VkImageUsageFlags usage, VkImageLayout initialImageLayout,
             VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageAspectFlags aspectFlags,
             VmaMemoryUsage memoryUsage)
    : mCurrentImageLayout(VK_IMAGE_LAYOUT_UNDEFINED), mLayerCount(1),
      mFormat(VK_FORMAT_R8G8B8A8_UNORM) {
  // load image from path
  int width, height, channels;
  auto *imageData = _loadImageFromPath(filename, width, height, channels);
  mWidth          = static_cast<uint32_t>(width);
  mHeight         = static_cast<uint32_t>(height);

  VkResult result = _createImage(numSamples, tiling, usage, memoryUsage);
  assert(result == VK_SUCCESS && "failed to create image!");

  // make it pastable
  if (initialImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    _transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  }

  // copy the image data to the image
  _copyDataToImage(imageData);

  // free the image data
  _freeImageData(imageData);

  if (initialImageLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
    _transitionImageLayout(initialImageLayout);
  }

  mVkImageView = createImageView(VulkanApplicationContext::getInstance()->getDevice(), mVkImage,
                                 mFormat, aspectFlags, mLayerCount);
}

Image::Image(const std::vector<std::string> &filenames, VkImageUsageFlags usage,
             VkImageLayout initialImageLayout, VkSampleCountFlagBits numSamples,
             VkImageTiling tiling, VkImageAspectFlags aspectFlags, VmaMemoryUsage memoryUsage)
    : mCurrentImageLayout(VK_IMAGE_LAYOUT_UNDEFINED), mLayerCount(filenames.size()),
      mFormat(VK_FORMAT_R8G8B8A8_UNORM) {
  std::vector<unsigned char *> imageDatas{};

  int width, height, channels;
  for (auto &filename : filenames) {
    // load image from path
    auto *imageData = _loadImageFromPath(filename, width, height, channels);
    imageDatas.push_back(imageData);
  }
  mWidth  = static_cast<uint32_t>(width);
  mHeight = static_cast<uint32_t>(height);

  VkResult result = _createImage(numSamples, tiling, usage, memoryUsage);
  assert(result == VK_SUCCESS && "failed to create image!");

  // make it pastable
  if (initialImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    _transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  }

  // copy the image data to the image
  for (uint32_t i = 0; i < filenames.size(); ++i) {
    _copyDataToImage(imageDatas[i], i);
    _freeImageData(imageDatas[i]);
  }

  if (initialImageLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
    _transitionImageLayout(initialImageLayout);
  }

  mVkImageView = createImageView(VulkanApplicationContext::getInstance()->getDevice(), mVkImage,
                                 mFormat, aspectFlags, mLayerCount);
}

Image::~Image() {
  if (mVkImage != VK_NULL_HANDLE) {
    vkDestroyImageView(VulkanApplicationContext::getInstance()->getDevice(), mVkImageView, nullptr);
    vkDestroyImage(VulkanApplicationContext::getInstance()->getDevice(), mVkImage, nullptr);
    vmaFreeMemory(VulkanApplicationContext::getInstance()->getAllocator(), mAllocation);
  }
}

void Image::clearImage(VkCommandBuffer commandBuffer) {
  VkImageSubresourceRange clearRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  vkCmdClearColorImage(commandBuffer, mVkImage, VK_IMAGE_LAYOUT_GENERAL, &kClearColor, 1,
                       &clearRange);
}

void Image::_copyDataToImage(unsigned char *imageData, uint32_t layerToCopyTo) {
  auto &device      = VulkanApplicationContext::getInstance()->getDevice();
  auto &queue       = VulkanApplicationContext::getInstance()->getGraphicsQueue();
  auto &commandPool = VulkanApplicationContext::getInstance()->getCommandPool();
  auto &allocator   = VulkanApplicationContext::getInstance()->getAllocator();

  const uint32_t imagePixelCount = mWidth * mHeight;
  // the channel count is ignored here, because the VkFormat is enough
  const uint32_t imageDataSize = imagePixelCount * kVkFormatBytesPerPixelMap.at(mFormat);

  // create a staging buffer
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = imageDataSize;
  bufferInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VkBuffer stagingBuffer;
  VmaAllocation stagingBufferAllocation;
  vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &stagingBuffer, &stagingBufferAllocation,
                  nullptr);

  // copy your data to the staging buffer
  void *mappedData;
  vmaMapMemory(allocator, stagingBufferAllocation, &mappedData);
  memcpy(mappedData, imageData, imageDataSize);
  vmaUnmapMemory(allocator, stagingBufferAllocation);

  // record a command to copy the buffer to the image
  VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands(
      VulkanApplicationContext::getInstance()->getDevice(),
      VulkanApplicationContext::getInstance()->getCommandPool());

  VkBufferImageCopy region{};
  region.bufferOffset                = 0;
  region.bufferRowLength             = 0; // If your data is tightly packed, this can be 0
  region.bufferImageHeight           = 0; // If your data is tightly packed, this can be 0
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel   = 0;
  // the first layer of the texture array that the data should be copied into
  region.imageSubresource.baseArrayLayer = layerToCopyTo;
  region.imageSubresource.layerCount     = 1;
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {mWidth, mHeight, 1};

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, mVkImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  RenderSystem::endSingleTimeCommands(device, commandPool, queue, commandBuffer);

  // Clean up the staging buffer
  vmaDestroyBuffer(VulkanApplicationContext::getInstance()->getAllocator(), stagingBuffer,
                   stagingBufferAllocation);
}

VkResult Image::_createImage(VkSampleCountFlagBits numSamples, VkImageTiling tiling,
                             VkImageUsageFlags usage, VmaMemoryUsage memoryUsage) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = mWidth;
  imageInfo.extent.height = mHeight;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = mLayerCount;
  imageInfo.format        = mFormat;
  imageInfo.tiling        = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = usage;
  imageInfo.samples       = numSamples;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage                   = memoryUsage;

  return vmaCreateImage(VulkanApplicationContext::getInstance()->getAllocator(), &imageInfo,
                        &vmaallocInfo, &mVkImage, &mAllocation, nullptr);
}

VkImageView Image::createImageView(VkDevice device, const VkImage &image, VkFormat format,
                                   VkImageAspectFlags aspectFlags, uint32_t layerCount) {
  VkImageView imageView{};

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;

  viewInfo.viewType = (layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  viewInfo.format   = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = layerCount;

  VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
  assert(result == VK_SUCCESS && "failed to create texture image view!");

  return imageView;
}

VkDescriptorImageInfo Image::getDescriptorInfo(VkImageLayout imageLayout) const {
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = imageLayout;
  imageInfo.imageView   = mVkImageView;
  return imageInfo;
}

void Image::_transitionImageLayout(VkImageLayout newLayout) {
  auto &device      = VulkanApplicationContext::getInstance()->getDevice();
  auto &queue       = VulkanApplicationContext::getInstance()->getGraphicsQueue();
  auto &commandPool = VulkanApplicationContext::getInstance()->getCommandPool();

  VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands(device, commandPool);

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = mCurrentImageLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = mVkImage;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = mLayerCount;

  VkPipelineStageFlags sourceStage      = VK_PIPELINE_STAGE_NONE;
  VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;
  if (mCurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (mCurrentImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (mCurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
              newLayout == VK_IMAGE_LAYOUT_GENERAL)) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  } else if (mCurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  } else if (mCurrentImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_GENERAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  } else {
    assert(false && "unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);

  RenderSystem::endSingleTimeCommands(device, commandPool, queue, commandBuffer);

  mCurrentImageLayout = newLayout;
}

namespace {
VkImageMemoryBarrier getMemoryBarrier(VkImage image, VkImageLayout oldLayout,
                                      VkImageLayout newLayout, VkAccessFlags srcAccessMask,
                                      VkAccessFlags dstAccessMask) {
  VkImageMemoryBarrier memoryBarrier = {};
  memoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  memoryBarrier.oldLayout            = oldLayout;
  memoryBarrier.newLayout            = newLayout;
  memoryBarrier.image                = image;
  memoryBarrier.subresourceRange     = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  memoryBarrier.srcAccessMask        = srcAccessMask;
  memoryBarrier.dstAccessMask        = dstAccessMask;
  return memoryBarrier;
}

VkImageCopy imageCopyRegion(uint32_t width, uint32_t height) {
  VkImageCopy region;
  region.dstOffset                     = {0, 0, 0};
  region.srcOffset                     = {0, 0, 0};
  region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.dstSubresource.mipLevel       = 0;
  region.dstSubresource.baseArrayLayer = 0;
  region.dstSubresource.layerCount     = 1;
  region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.srcSubresource.mipLevel       = 0;
  region.srcSubresource.baseArrayLayer = 0;
  region.srcSubresource.layerCount     = 1;
  region.extent                        = {width, height, 1};
  return region;
}
} // namespace

ImageForwardingPair::ImageForwardingPair(VkImage image1, VkImage image2,
                                         VkImageLayout image1BeforeCopy,
                                         VkImageLayout image2BeforeCopy,
                                         VkImageLayout image1AfterCopy,
                                         VkImageLayout image2AfterCopy)
    : mImage1(image1), mImage2(image2) {
  mCopyRegion =
      imageCopyRegion(VulkanApplicationContext::getInstance()->getSwapchainExtentWidth(),
                      VulkanApplicationContext::getInstance()->getSwapchainExtentHeight());

  mImage1BeforeCopy = getMemoryBarrier(
      image1, image1BeforeCopy, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
  mImage2BeforeCopy = getMemoryBarrier(
      image2, image2BeforeCopy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
  mImage1AfterCopy = getMemoryBarrier(image1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image1AfterCopy,
                                      VK_ACCESS_TRANSFER_READ_BIT,
                                      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
  mImage2AfterCopy = getMemoryBarrier(image2, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image2AfterCopy,
                                      VK_ACCESS_TRANSFER_WRITE_BIT,
                                      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
}

void ImageForwardingPair::forwardCopying(VkCommandBuffer commandBuffer) {
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &mImage1BeforeCopy);
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &mImage2BeforeCopy);
  vkCmdCopyImage(commandBuffer, mImage1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mImage2,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &mCopyRegion);
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &mImage1AfterCopy);
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &mImage2AfterCopy);
}