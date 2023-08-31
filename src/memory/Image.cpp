#include "Image.h"

#include "Buffer.h"
#include "render-context/RenderSystem.h"
// #include "utils/StbImageImpl.h"
#include "utils/logger.h"
#include "vulkan/vulkan_core.h"

#include <cmath>
#include <iostream>
#include <string>

VkImageView Image::createImageView(const VkImage &image, VkFormat format, VkImageAspectFlags aspectFlags) {
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

  VkResult result =
      vkCreateImageView(VulkanApplicationContext::getInstance()->getDevice(), &viewInfo, nullptr, &imageView);
  logger::checkStep("vkCreateImageView", result);

  return imageView;
}

VkResult Image::createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, uint32_t mipLevels,
                            VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
                            VkImageLayout initialImageLayout) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = mipLevels;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = format;
  imageInfo.tiling        = tiling;
  imageInfo.initialLayout = initialImageLayout;
  imageInfo.usage         = usage;
  imageInfo.samples       = numSamples;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage                   = memoryUsage;

  return vmaCreateImage(VulkanApplicationContext::getInstance()->getAllocator(), &imageInfo, &vmaallocInfo, &mVkImage,
                        &mAllocation, nullptr);
}

Image::Image(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
             VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, VmaMemoryUsage memoryUsage,
             VkImageLayout initialImageLayout)
    : mCurrentImageLayout(initialImageLayout), mWidth(width), mHeight(height) {
  VkResult result =
      createImage(width, height, numSamples, mipLevels, format, tiling, usage, memoryUsage, initialImageLayout);
  if (result != VK_SUCCESS) {
    logger::throwError("failed to create image!"); //  checkstep is not used here because if should be used to avoid
                                                   //  clang-tidy warnings
  }
  mVkImageView = createImageView(mVkImage, format, aspectFlags);
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

void Image::transitionImageLayout(VkImageLayout newLayout, uint32_t mipLevels) {
  VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands();
  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = mCurrentImageLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = mVkImage;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  VkPipelineStageFlags sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  if (mCurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (mCurrentImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (mCurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_GENERAL)) {
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    destinationStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  } else {
    logger::throwError("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  RenderSystem::endSingleTimeCommands(commandBuffer);

  mCurrentImageLayout = newLayout;
}

void Image::copyBufferToImage(const VkBuffer &buffer, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands();
  VkBufferImageCopy region{};
  region.bufferOffset      = 0;
  region.bufferRowLength   = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, mVkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  RenderSystem::endSingleTimeCommands(commandBuffer);
}

// void Image::generateMipmaps(VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
//   // Check if image format supports linear blitting
//   VkFormatProperties formatProperties;
//   vkGetPhysicalDeviceFormatProperties(VulkanApplicationContext::getInstance()->getPhysicalDevice(), imageFormat,
//   &formatProperties);

//   if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
//     logger::throwError("texture image format does not support linear blitting!");
//   }

//   VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands();

//   VkImageMemoryBarrier barrier{};
//   barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//   barrier.image                           = mVkImage;
//   barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
//   barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
//   barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
//   barrier.subresourceRange.baseArrayLayer = 0;
//   barrier.subresourceRange.layerCount     = 1;
//   barrier.subresourceRange.levelCount     = 1;

//   int32_t mipWidth  = texWidth;
//   int32_t mipHeight = texHeight;

//   for (uint32_t i = 1; i < mipLevels; i++) {
//     barrier.subresourceRange.baseMipLevel = i - 1;
//     barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//     barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//     barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
//     barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

//     vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
//     nullptr,
//                          0, nullptr, 1, &barrier);

//     VkImageBlit blit{};
//     blit.srcOffsets[0]                 = {0, 0, 0};
//     blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
//     blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
//     blit.srcSubresource.mipLevel       = i - 1;
//     blit.srcSubresource.baseArrayLayer = 0;
//     blit.srcSubresource.layerCount     = 1;
//     blit.dstOffsets[0]                 = {0, 0, 0};
//     blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
//     blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
//     blit.dstSubresource.mipLevel       = i;
//     blit.dstSubresource.baseArrayLayer = 0;
//     blit.dstSubresource.layerCount     = 1;

//     vkCmdBlitImage(commandBuffer, mVkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mVkImage,
//                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

//     barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//     barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//     barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//     barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

//     vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
//                          nullptr, 0, nullptr, 1, &barrier);

//     if (mipWidth > 1) mipWidth /= 2;
//     if (mipHeight > 1) mipHeight /= 2;
//   }

//   barrier.subresourceRange.baseMipLevel = mipLevels - 1;
//   barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//   barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//   barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
//   barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

//   vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
//                        nullptr, 0, nullptr, 1, &barrier);

//   RenderSystem::endSingleTimeCommands(commandBuffer);
// }

// void Image::createTextureImage(const std::string &path, std::shared_ptr<Image> allocatedImage,
//                                uint32_t mipLevels) {
//   int texWidth, texHeight, texChannels;
//   StbImageImpl tex(path, texWidth, texHeight, texChannels);

//   mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

//   VkDeviceSize imageSize = texWidth * texHeight * 4;

//   if (!tex.pixels) {
//     logger::throwError("failed to load texture image!");
//   }

//   Buffer stagingBuffer{imageSize * sizeof(unsigned char), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//   VMA_MEMORY_USAGE_CPU_TO_GPU,
//                        tex.pixels};

//   createImage(texWidth, texHeight, VK_SAMPLE_COUNT_1_BIT, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
//   VK_IMAGE_TILING_OPTIMAL,
//               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
//               VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, allocatedImage);
//   std::cout << "creating texture" << std::endl;
//   transitionImageLayout(allocatedImage->mVkImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
//                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
//   copyBufferToImage(stagingBuffer.getVkBuffer(), allocatedImage->mVkImage, static_cast<uint32_t>(texWidth),
//                     static_cast<uint32_t>(texHeight));
//   generateMipmaps(allocatedImage->mVkImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
//   // stagingBuffer.destroy();
// }

// void Image::createTextureSampler(std::shared_ptr<VkSampler> textureSampler, uint32_t mipLevels) {
//   VkSamplerCreateInfo samplerInfo{};
//   samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//   samplerInfo.magFilter               = VK_FILTER_LINEAR;
//   samplerInfo.minFilter               = VK_FILTER_LINEAR;
//   samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//   samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//   samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//   samplerInfo.anisotropyEnable        = VK_FALSE;
//   samplerInfo.maxAnisotropy           = 1.0f;
//   samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
//   samplerInfo.unnormalizedCoordinates = VK_FALSE;
//   samplerInfo.compareEnable           = VK_FALSE;
//   samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
//   samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//   samplerInfo.minLod                  = 0.0f; // Optional
//   samplerInfo.maxLod                  = static_cast<float>(mipLevels);
//   samplerInfo.mipLodBias              = 0.0f; // Optional

//   VkResult result = vkCreateSampler(VulkanApplicationContext::getInstance()->getDevice(), &samplerInfo, nullptr,
//   textureSampler.get()); logger::checkStep("vkCreateSampler", result);
// }

// Texture::Texture(const std::string &path) {
//   mImage   = std::make_shared<Image>();
//   mSampler = std::make_shared<VkSampler>();

//   mImage->createTextureImage(path, mMips);
//   mImage->createTextureSampler(mSampler, mMips);
// }

// Texture::Texture(const std::shared_ptr<Image> &image) : mImage(image) {
//   mSampler = std::make_shared<VkSampler>();
//   ImageUtils::createTextureSampler(mSampler, mMips);
// }

// Texture::~Texture() { vkDestroySampler(VulkanApplicationContext::getInstance()->getDevice(), *mSampler, nullptr); }

// VkDescriptorImageInfo Texture::getDescriptorInfo() {
//   VkDescriptorImageInfo imageInfo{};
//   imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//   imageInfo.imageView   = mImage->mVkImageView;
//   imageInfo.sampler     = *mSampler;

//   return imageInfo;
// }