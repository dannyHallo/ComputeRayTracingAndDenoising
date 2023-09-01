#include "Image.h"

#include "Buffer.h"
#include "render-context/RenderSystem.h"
#include "utils/logger.h"
#include "vulkan/vulkan_core.h"

#include <cmath>
#include <iostream>
#include <string>

VkImageView Image::createImageView(VkDevice device, const VkImage &image, VkFormat format,
                                   VkImageAspectFlags aspectFlags) {
  VkImageView imageView{};

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
  logger::checkStep("vkCreateImageView", result);

  return imageView;
}

VkResult Image::createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format,
                            VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = format;
  imageInfo.tiling        = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = usage;
  imageInfo.samples       = numSamples;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage                   = memoryUsage;

  return vmaCreateImage(VulkanApplicationContext::getInstance()->getAllocator(), &imageInfo, &vmaallocInfo, &mVkImage,
                        &mAllocation, nullptr);
}

Image::Image(VkDevice device, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height,
             VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
             VkImageAspectFlags aspectFlags, VmaMemoryUsage memoryUsage, VkImageLayout initialImageLayout)
    : mCurrentImageLayout(VK_IMAGE_LAYOUT_UNDEFINED), mWidth(width), mHeight(height) {
  VkResult result = createImage(width, height, numSamples, format, tiling, usage, memoryUsage);
  if (result != VK_SUCCESS) {
    logger::throwError("failed to create image!");
  }

  if (initialImageLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
    transitionImageLayout(device, commandPool, queue, initialImageLayout);
  }
  mVkImageView = createImageView(device, mVkImage, format, aspectFlags);
}

Image::~Image() {
  if (mVkImage != VK_NULL_HANDLE) {
    vkDestroyImageView(VulkanApplicationContext::getInstance()->getDevice(), mVkImageView, nullptr);
    vkDestroyImage(VulkanApplicationContext::getInstance()->getDevice(), mVkImage, nullptr);
    vmaFreeMemory(VulkanApplicationContext::getInstance()->getAllocator(), mAllocation);
  }
}

VkDescriptorImageInfo Image::getDescriptorInfo(VkImageLayout imageLayout) const {
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = imageLayout;
  imageInfo.imageView   = mVkImageView;
  return imageInfo;
}

void Image::transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImageLayout newLayout) {
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
  barrier.subresourceRange.layerCount     = 1;

  VkPipelineStageFlags sourceStage      = VK_PIPELINE_STAGE_NONE;
  VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;
  if (mCurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
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
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  } else if (mCurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  } else {
    logger::throwError("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  RenderSystem::endSingleTimeCommands(device, commandPool, queue, commandBuffer);

  mCurrentImageLayout = newLayout;
}