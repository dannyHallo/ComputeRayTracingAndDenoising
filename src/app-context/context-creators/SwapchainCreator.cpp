#include "SwapchainCreator.hpp"

#include "utils/logger/Logger.hpp"

#include <cassert>
#include <unordered_set>

namespace {
std::string _vkPresentModeKHRToString(VkPresentModeKHR presentMode) {
  switch (presentMode) {
  case VK_PRESENT_MODE_IMMEDIATE_KHR:
    return "VK_PRESENT_MODE_IMMEDIATE_KHR";
  case VK_PRESENT_MODE_MAILBOX_KHR:
    return "VK_PRESENT_MODE_MAILBOX_KHR";
  case VK_PRESENT_MODE_FIFO_KHR:
    return "VK_PRESENT_MODE_FIFO_KHR";
  case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
  case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
    return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
  case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
    return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
  case VK_PRESENT_MODE_MAX_ENUM_KHR:
    return "VK_PRESENT_MODE_MAX_ENUM_KHR";
  default:
    return "Unknown VkPresentModeKHR";
  }
}

VkImageView _createImageView(VkDevice device, const VkImage &image, VkFormat format,
                             VkImageAspectFlags aspectFlags, uint32_t imageDepth,
                             uint32_t layerCount) {

  VkImageView imageView{};

  VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
  if (layerCount == 1) {
    viewType = imageDepth > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D;
  } else {
    assert(imageDepth == 1 && "imageDepth must be 1 for 2D array images");
    viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  }

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = viewType;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = layerCount;

  vkCreateImageView(device, &viewInfo, nullptr, &imageView);

  return imageView;
}

// query for physical device's swapchain sepport details
ContextCreator::SwapchainSupportDetails _querySwapchainSupport(VkSurfaceKHR surface,
                                                               VkPhysicalDevice physicalDevice) {
  // get swapchain support details using surface and device info
  ContextCreator::SwapchainSupportDetails details;

  // get capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

  // get surface format
  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         details.formats.data());
  }

  // get available presentation modes
  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount,
                                              details.presentModes.data());
  }

  return details;
}

// find the most suitable swapchain surface format
VkSurfaceFormatKHR
_chooseSwapSurfaceFormat(Logger *logger, const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    // format: VK_FORMAT_B8G8R8A8_SRGB
    // this is actually irretional due to imgui impl
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
      return availableFormat;
    }
  }

  logger->info("Surface format requirement didn't meet, the first available format is chosen!");
  return availableFormats[0];
}

// choose the present mode of the swapchain
VkPresentModeKHR
_chooseSwapPresentMode(Logger *logger, bool isFramerateLimited,
                       const std::vector<VkPresentModeKHR> &availablePresentModes) {
  VkPresentModeKHR preferredPresentMode =
      isFramerateLimited ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_MAILBOX_KHR;

  std::unordered_set<VkPresentModeKHR> availablePresentModesSet(availablePresentModes.begin(),
                                                                availablePresentModes.end());

  if (availablePresentModesSet.find(preferredPresentMode) != availablePresentModesSet.end()) {
    return preferredPresentMode;
  }

  logger->info("optimized present mode doesn't meet, switching to VK_PRESENT_MODE_FIFO_KHR");
  return VK_PRESENT_MODE_FIFO_KHR;
}

// return the current extent, or create another one
VkExtent2D _getSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, Logger *logger) {
  assert(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() &&
         "currentExtent.width should be valid!");

  logger->info("using resolution: ({}, {})", capabilities.currentExtent.width,
               capabilities.currentExtent.height);

  return capabilities.currentExtent;
}
} // namespace

void ContextCreator::createSwapchain(Logger *logger, bool isFramerateLimited,
                                     VkSwapchainKHR &swapchain,
                                     std::vector<VkImage> &swapchainImages,
                                     std::vector<VkImageView> &swapchainImageViews,
                                     VkFormat &swapchainImageFormat, VkExtent2D &swapchainExtent,
                                     const VkSurfaceKHR &surface, const VkDevice &device,
                                     const VkPhysicalDevice &physicalDevice,
                                     const QueueFamilyIndices &queueFamilyIndices) {
  SwapchainSupportDetails swapchainSupport = _querySwapchainSupport(surface, physicalDevice);
  VkSurfaceFormatKHR surfaceFormat = _chooseSwapSurfaceFormat(logger, swapchainSupport.formats);
  swapchainImageFormat             = surfaceFormat.format;

  logger->info("all present modes", swapchainSupport.presentModes.size());
  for (auto const &mode : swapchainSupport.presentModes) {
    logger->subInfo("{}", _vkPresentModeKHRToString(mode));
  }
  logger->println();

  VkPresentModeKHR presentMode =
      _chooseSwapPresentMode(logger, isFramerateLimited, swapchainSupport.presentModes);

  swapchainExtent = _getSwapExtent(swapchainSupport.capabilities, logger);

  // recommanded: min + 1
  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
  // otherwise: max
  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }
  logger->info("number of swapchain images: {}", imageCount);

  VkSwapchainCreateInfoKHR swapchainCreateInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainCreateInfo.surface          = surface;
  swapchainCreateInfo.minImageCount    = imageCount;
  swapchainCreateInfo.imageFormat      = surfaceFormat.format;
  swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
  swapchainCreateInfo.imageExtent      = swapchainExtent;
  swapchainCreateInfo.imageArrayLayers = 1; // the amount of layers each image consists of
  swapchainCreateInfo.imageUsage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  std::vector<uint32_t> queueFamilyIndicesArray = {queueFamilyIndices.graphicsFamily,
                                                   queueFamilyIndices.presentFamily};

  if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
    // images can be used across multiple queue families without explicit
    // ownership transfers.
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainCreateInfo.queueFamilyIndexCount =
        static_cast<uint32_t>(queueFamilyIndicesArray.size());
    swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndicesArray.data();
  } else {
    // an image is owned by one queue family at a time and ownership must be
    // explicitly transferred before the image is being used in another
    // queue family. This offers the best performance.
    swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;       // Optional
    swapchainCreateInfo.pQueueFamilyIndices   = nullptr; // Optional
  }

  swapchainCreateInfo.preTransform   = swapchainSupport.capabilities.currentTransform;
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  swapchainCreateInfo.presentMode = presentMode;
  swapchainCreateInfo.clipped     = VK_TRUE;

  swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);

  // obtain the actual number of swapchain images
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  swapchainImageViews.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

  for (size_t i = 0; i < imageCount; i++) {
    swapchainImageViews[i] = _createImageView(device, swapchainImages[i], swapchainImageFormat,
                                              VK_IMAGE_ASPECT_COLOR_BIT, 1, 1);
  }
}
