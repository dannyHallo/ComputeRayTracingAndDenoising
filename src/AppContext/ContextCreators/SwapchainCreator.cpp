#include "SwapchainCreator.h"

#include "memory/Image.h"
#include "utils/Logger.h"

namespace {
// query for physical device's swapchain sepport details
SwapchainSupportDetails querySwapchainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice) {
  // get swapchain support details using surface and device info
  SwapchainSupportDetails details;

  // get capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

  // get surface format
  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
  }

  // get available presentation modes
  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

// find the most suitable swapchain surface format
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    // format: VK_FORMAT_B8G8R8A8_SRGB
    // this is actually irretional due to imgui impl
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB) {
      return availableFormat;
    }
  }

  Logger::print("Surface format requirement didn't meet, the first available format is chosen!");
  return availableFormats[0];
}

// choose the present mode of the swapchain
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
  // our preferance: Mailbox present mode
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  Logger::print("Present mode preferance doesn't meet, switching to FIFO");
  return VK_PRESENT_MODE_FIFO_KHR;
}

// return the current extent, or create another one
VkExtent2D getSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  // if the current extent is valid (we can read the width and height out of it)
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    std::cout << "Using resolution: (" << capabilities.currentExtent.width << ", " << capabilities.currentExtent.height
              << ")" << std::endl;
    return capabilities.currentExtent;
  }

  Logger::throwError("This part shouldn't be reached!");
  return {};
}

} // namespace

void SwapchainCreator::create(VkSwapchainKHR &swapchain, std::vector<VkImage> &swapchainImages,
                              std::vector<VkImageView> &swapchainImageViews, VkFormat &swapchainImageFormat,
                              VkExtent2D &swapchainExtent, const VkSurfaceKHR &surface, const VkDevice &device,
                              const VkPhysicalDevice &physicalDevice, const QueueFamilyIndices &queueFamilyIndices) {
  SwapchainSupportDetails swapchainSupport = querySwapchainSupport(surface, physicalDevice);
  VkSurfaceFormatKHR surfaceFormat         = chooseSwapSurfaceFormat(swapchainSupport.formats);
  swapchainImageFormat                     = surfaceFormat.format;
  VkPresentModeKHR presentMode             = chooseSwapPresentMode(swapchainSupport.presentModes);
  swapchainExtent                          = getSwapExtent(swapchainSupport.capabilities);

  // recommanded: min + 1
  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
  // otherwise: max
  if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }
  Logger::print("number of swapchain images", imageCount);

  VkSwapchainCreateInfoKHR swapchainCreateInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainCreateInfo.surface          = surface;
  swapchainCreateInfo.minImageCount    = imageCount;
  swapchainCreateInfo.imageFormat      = surfaceFormat.format;
  swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
  swapchainCreateInfo.imageExtent      = swapchainExtent;
  swapchainCreateInfo.imageArrayLayers = 1; // the amount of layers each image consists of
  swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  uint32_t queueFamilyIndicesArray[] = {queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily};

  if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
    // images can be used across multiple queue families without explicit ownership transfers.
    swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    swapchainCreateInfo.queueFamilyIndexCount = 2;
    swapchainCreateInfo.pQueueFamilyIndices   = static_cast<const uint32_t *>(queueFamilyIndicesArray);
  } else {
    // an image is owned by one queue family at a time and ownership must be explicitly transferred before the image is
    // being used in another queue family. This offers the best performance.
    swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;       // Optional
    swapchainCreateInfo.pQueueFamilyIndices   = nullptr; // Optional
  }

  swapchainCreateInfo.preTransform   = swapchainSupport.capabilities.currentTransform;
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  swapchainCreateInfo.presentMode = presentMode;
  swapchainCreateInfo.clipped     = VK_TRUE;

  swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
  Logger::checkStep("vkCreateSwapchainKHR", result);

  // obtain the actual number of swapchain images
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  swapchainImageViews.reserve(imageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

  for (size_t i = 0; i < imageCount; i++) {
    swapchainImageViews.emplace_back(
        Image::createImageView(device, swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT));
  }
}