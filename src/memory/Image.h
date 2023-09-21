#pragma once

#include "utils/vulkan.h"
#include "vk_mem_alloc.h"

#include <iostream>
#include <memory>
#include <string>

// the wrapper class of VkImage and its corresponding VkImageView, handles
// memory allocation
class Image {
  VkImage mVkImage          = VK_NULL_HANDLE;
  VkImageView mVkImageView  = VK_NULL_HANDLE;
  VmaAllocation mAllocation = VK_NULL_HANDLE;
  VkImageLayout mCurrentImageLayout;
  uint32_t mWidth;
  uint32_t mHeight;

public:
  // constructor when a new VkImage and VkImageView should be created
  Image(VkDevice device, VkCommandPool commandPool, VkQueue queue,
        uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkImageAspectFlags aspectFlags, VmaMemoryUsage memoryUsage,
        VkImageLayout initialImageLayout = VK_IMAGE_LAYOUT_GENERAL);

  ~Image();

  // disable move and copy
  Image(const Image &)            = delete;
  Image &operator=(const Image &) = delete;
  Image(Image &&)                 = delete;
  Image &operator=(Image &&)      = delete;

  VkImage &getVkImage() { return mVkImage; }
  VmaAllocation &getAllocation() { return mAllocation; }
  VkImageView &getImageView() { return mVkImageView; }
  [[nodiscard]] VkDescriptorImageInfo
  getDescriptorInfo(VkImageLayout imageLayout) const;
  [[nodiscard]] uint32_t getWidth() const { return mWidth; }
  [[nodiscard]] uint32_t getHeight() const { return mHeight; }

  void transitionImageLayout(VkDevice device, VkCommandPool commandPool,
                             VkQueue queue, VkImageLayout newLayout);

  // void copyBufferToImage(const VkBuffer &buffer, uint32_t width, uint32_t
  // height);

  static VkImageView createImageView(VkDevice device, const VkImage &image,
                                     VkFormat format,
                                     VkImageAspectFlags aspectFlags);

private:
  // creates an image with VK_IMAGE_LAYOUT_UNDEFINED initially
  VkResult createImage(uint32_t width, uint32_t height,
                       VkSampleCountFlagBits numSamples, VkFormat format,
                       VkImageTiling tiling, VkImageUsageFlags usage,
                       VmaMemoryUsage memoryUsage);
};