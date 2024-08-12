#pragma once

#include "volk.h"

#ifdef _WIN32
#include "vma/vk_mem_alloc.h" // NO_G3_REWRITE
#else
#include "vk_mem_alloc.h" // NO_G3_REWRITE
#endif

#include <string>
#include <vector>

struct ImageDimensions {
  uint32_t width;
  uint32_t height;
  uint32_t depth;

  ImageDimensions() : width(0), height(0), depth(0) {}

  ImageDimensions(uint32_t width, uint32_t height, uint32_t depth)
      : width(width), height(height), depth(depth) {}

  ImageDimensions(uint32_t width, uint32_t height) : ImageDimensions(width, height, 1) {}

  bool operator==(ImageDimensions const &rhs) const {
    return width == rhs.width && height == rhs.height && depth == rhs.depth;
  }
};

class VulkanApplicationContext;
// the wrapper class of VkImage and its corresponding VkImageView, handles
// memory allocation
class Image {
public:
  // create a blank image
  Image(VulkanApplicationContext *appContext, ImageDimensions dimensions, VkFormat format,
        VkImageUsageFlags usage, VkSampler sampler = VK_NULL_HANDLE,
        VkImageLayout initialImageLayout = VK_IMAGE_LAYOUT_GENERAL,
        VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT,
        VkImageTiling tiling             = VK_IMAGE_TILING_OPTIMAL,
        VkImageAspectFlags aspectFlags   = VK_IMAGE_ASPECT_COLOR_BIT);

  // create an image from a file, VK_FORMAT_R8G8B8A8_UNORM is the only format that stb_image
  // supports, so the created image format is fixed, and only 2D images are supported.
  Image(VulkanApplicationContext *appContext, const std::string &filename, VkImageUsageFlags usage,
        VkSampler sampler                = VK_NULL_HANDLE,
        VkImageLayout initialImageLayout = VK_IMAGE_LAYOUT_GENERAL,
        VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT,
        VkImageTiling tiling             = VK_IMAGE_TILING_OPTIMAL,
        VkImageAspectFlags aspectFlags   = VK_IMAGE_ASPECT_COLOR_BIT);

  // create a texture array from a set of image files, all images should be in
  // the same dimension and the same format..
  Image(VulkanApplicationContext *appContext, const std::vector<std::string> &filenames,
        VkImageUsageFlags usage, VkSampler sampler = VK_NULL_HANDLE,
        VkImageLayout initialImageLayout = VK_IMAGE_LAYOUT_GENERAL,
        VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT,
        VkImageTiling tiling             = VK_IMAGE_TILING_OPTIMAL,
        VkImageAspectFlags aspectFlags   = VK_IMAGE_ASPECT_COLOR_BIT);

  ~Image();

  // disable move and copy
  Image(const Image &)            = delete;
  Image &operator=(const Image &) = delete;
  Image(Image &&)                 = delete;
  Image &operator=(Image &&)      = delete;

  VkImage &getVkImage() { return _vkImage; }

  [[nodiscard]] VkDescriptorImageInfo getDescriptorInfo(VkImageLayout imageLayout) const;
  [[nodiscard]] ImageDimensions getDimensions() const { return _dimensions; }

  void clearImage(VkCommandBuffer commandBuffer);

  static VkImageView createImageView(VkDevice device, const VkImage &image, VkFormat format,
                                     VkImageAspectFlags aspectFlags, uint32_t imageDepth = 1,
                                     uint32_t layerCount = 1);

private:
  VulkanApplicationContext *_appContext;

  VkImage _vkImage          = VK_NULL_HANDLE;
  VkImageView _vkImageView  = VK_NULL_HANDLE;
  VkSampler _vkSampler      = VK_NULL_HANDLE;
  VmaAllocation _allocation = VK_NULL_HANDLE;
  VkImageLayout _currentImageLayout;
  uint32_t _layerCount;
  VkFormat _format;

  ImageDimensions _dimensions;

  void _copyDataToImage(unsigned char *imageData, uint32_t layerToCopyTo = 0);

  // creates an image with VK_IMAGE_LAYOUT_UNDEFINED initially
  VkResult _createImage(VkSampleCountFlagBits numSamples, VkImageTiling tiling,
                        VkImageUsageFlags usage);

  void _transitionImageLayout(VkImageLayout newLayout);
};

// storing the pointer of a pair of imgs, support for easy dumping
class ImageForwardingPair {
public:
  // if the image wrapper is given, the dimensions information is stored alongside
  ImageForwardingPair(Image *image1, Image *image2, VkImageLayout image1BeforeCopy,
                      VkImageLayout image2BeforeCopy, VkImageLayout image1AfterCopy,
                      VkImageLayout image2AfterCopy);

  // when VkImage is given, explicit dimensions information is expected
  ImageForwardingPair(VkImage image1, VkImage image2, ImageDimensions dimensions,
                      VkImageLayout image1BeforeCopy, VkImageLayout image2BeforeCopy,
                      VkImageLayout image1AfterCopy, VkImageLayout image2AfterCopy);

  void forwardCopy(VkCommandBuffer commandBuffer);

private:
  VkImage _image1 = VK_NULL_HANDLE;
  VkImage _image2 = VK_NULL_HANDLE;

  VkImageCopy _copyRegion{};
  VkImageMemoryBarrier _image1BeforeCopy{};
  VkImageMemoryBarrier _image2BeforeCopy{};
  VkImageMemoryBarrier _image1AfterCopy{};
  VkImageMemoryBarrier _image2AfterCopy{};

  void _init(VkImage image1, VkImage image2, ImageDimensions dimensions,
             VkImageLayout image1BeforeCopy, VkImageLayout image2BeforeCopy,
             VkImageLayout image1AfterCopy, VkImageLayout image2AfterCopy);
};