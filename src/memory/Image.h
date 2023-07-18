#pragma once

#include "app-context/VulkanApplicationContext.h"
// #include "vk_mem_alloc.h" // TODO: why is required?

#include <iostream>
#include <memory>
#include <string>

class Image {
  VkImage mVkImage          = VK_NULL_HANDLE;
  VkImageView mVkImageView  = VK_NULL_HANDLE;
  VmaAllocation mAllocation = VK_NULL_HANDLE;
  VkImageLayout mCurrentImageLayout;
  uint32_t mWidth;
  uint32_t mHeight;

public:
  // constructor when a new VkImage and VkImageView should be created
  Image(uint32_t width, uint32_t height, const uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, VmaMemoryUsage memoryUsage,
        VkImageLayout initialImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

  ~Image();

  VkImage &getVkImage() { return mVkImage; }
  VmaAllocation &getAllocation() { return mAllocation; }
  VkImageView &getImageView() { return mVkImageView; }
  VkDescriptorImageInfo getDescriptorInfo(VkImageLayout imageLayout) const;
  const uint32_t getWidth() const { return mWidth; }
  const uint32_t getHeight() const { return mHeight; }

  void transitionImageLayout(VkImageLayout newLayout, const uint32_t mipLevels = 1);

  void copyBufferToImage(const VkBuffer &buffer, uint32_t width, uint32_t height);

  void generateMipmaps(VkFormat imageFormat, int32_t texWidth, int32_t texHeight, const uint32_t mipLevels);

  void createTextureImage(const std::string &path, Image &allocatedImage, const uint32_t mipLevels);

  void createTextureSampler(std::shared_ptr<VkSampler> textureSampler, const uint32_t mipLevels);

  static VkImageView createImageView(const VkImage &image, VkFormat format, VkImageAspectFlags aspectFlags,
                                     const uint32_t &mipLevels);
};

// class Texture {
//   std::shared_ptr<Image> mImage;
//   std::shared_ptr<VkSampler> mSampler;
//   uint32_t mMips = 1;

// public:
//   Texture(const std::string &path);
//   Texture(const std::shared_ptr<Image> &image);

//   ~Texture();

//   std::shared_ptr<Image> getImage() { return mImage; }
//   std::shared_ptr<VkSampler> getSampler() { return mSampler; }
//   VkDescriptorImageInfo getDescriptorInfo();
// };