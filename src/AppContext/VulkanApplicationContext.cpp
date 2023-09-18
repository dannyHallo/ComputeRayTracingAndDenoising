#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

// VOLK_IMPLEMENTATION lets volk define the functions, by letting volk.h include volk.c
// this must only be defined in one translation unit
#define VOLK_IMPLEMENTATION
#include "AppContext/VulkanApplicationContext.h"

#include "utils/logger.h"

#include "memory/Image.h"

#include "ContextCreators/DeviceCreator.h"
#include "ContextCreators/InstanceCreator.h"
#include "ContextCreators/SurfaceCreator.h"

#include <algorithm>
#include <cstdint>
#include <set>

static const std::vector<const char *> validationLayers         = {"VK_LAYER_KHRONOS_validation"};
static const std::vector<const char *> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static const bool enableDebug                                   = true;

std::unique_ptr<VulkanApplicationContext> VulkanApplicationContext::sInstance = nullptr;

VulkanApplicationContext *VulkanApplicationContext::initInstance(GLFWwindow *window) {
  if (sInstance != nullptr) {
    logger::throwError("VulkanApplicationContext::initInstance: instance is already initialized");
  }
  sInstance = std::unique_ptr<VulkanApplicationContext>(new VulkanApplicationContext(window));
  return sInstance.get();
}

void VulkanApplicationContext::destroyInstance() {
  if (sInstance == nullptr) {
    logger::throwError("VulkanApplicationContext::destroyInstance: instance is not initialized");
  }
  sInstance.reset(nullptr);
}

VulkanApplicationContext *VulkanApplicationContext::getInstance() {
  if (sInstance == nullptr) {
    logger::throwError("VulkanApplicationContext::getInstance: instance is not initialized");
  }
  return sInstance.get();
}

VulkanApplicationContext::VulkanApplicationContext(GLFWwindow *glWindow) : mGlWindow(glWindow) {
  VkResult result = volkInitialize();
  logger::checkStep("volkInitialize", result);

  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName   = "Compute Ray Tracing";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = "No Engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_2;
  InstanceCreator::create(mVkInstance, mDebugMessager, appInfo, validationLayers);

  // load instance related functions
  volkLoadInstance(mVkInstance);

  SurfaceCreator::create(mVkInstance, mSurface, mGlWindow);

  // createDevice();
  DeviceCreator::create(mPhysicalDevice, mDevice, mQueueFamilyIndices, mGraphicsQueue, mPresentQueue, mComputeQueue,
                        mTransferQueue, mVkInstance, mSurface, requiredDeviceExtensions);

  // reduce loading overhead by specifing only one device is used
  volkLoadDevice(mDevice);

  createSwapchain();

  createAllocator();
  createCommandPool();
}

VulkanApplicationContext::~VulkanApplicationContext() {
  logger::print("Destroying VulkanApplicationContext");

  vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
  vkDestroyCommandPool(mDevice, mGuiCommandPool, nullptr);

  for (auto &swapchainImageView : mSwapchainImageViews) {
    vkDestroyImageView(mDevice, swapchainImageView, nullptr);
  }

  vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

  vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);

  // this step destroys allocated VkDestroyMemory allocated by VMA when creating buffers and images,
  // by destroying the global allocator
  vmaDestroyAllocator(mAllocator);
  vkDestroyDevice(mDevice, nullptr);

  if (enableDebug) {
    vkDestroyDebugUtilsMessengerEXT(mVkInstance, mDebugMessager, nullptr);
  }

  vkDestroyInstance(mVkInstance, nullptr);
}

// query for physical device's swapchain sepport details
SwapchainSupportDetails VulkanApplicationContext::querySwapchainSupport(VkSurfaceKHR surface,
                                                                        VkPhysicalDevice physicalDevice) {
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

  logger::print("Surface format requirement didn't meet, the first available format is chosen!");
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

  logger::print("Present mode preferance doesn't meet, switching to FIFO");
  return VK_PRESENT_MODE_FIFO_KHR;
}

// return the current extent, or create another one
VkExtent2D VulkanApplicationContext::getSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  // if the current extent is valid (we can read the width and height out of it)
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    std::cout << "Using resolution: (" << capabilities.currentExtent.width << ", " << capabilities.currentExtent.height
              << ")" << std::endl;
    return capabilities.currentExtent;
  }

  logger::throwError("This part shouldn't be reached!");
  return {};
}

// create swapchain and swapchain imageviews
void VulkanApplicationContext::createSwapchain() {
  SwapchainSupportDetails swapchainSupport = querySwapchainSupport(mSurface, mPhysicalDevice);
  VkSurfaceFormatKHR surfaceFormat         = chooseSwapSurfaceFormat(swapchainSupport.formats);
  mSwapchainImageFormat                    = surfaceFormat.format;
  VkPresentModeKHR presentMode             = chooseSwapPresentMode(swapchainSupport.presentModes);
  mSwapchainExtent                         = getSwapExtent(swapchainSupport.capabilities);

  // recommanded: min + 1
  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
  // otherwise: max
  if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }
  logger::print("number of swapchain images", imageCount);

  VkSwapchainCreateInfoKHR swapchainCreateInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainCreateInfo.surface          = mSurface;
  swapchainCreateInfo.minImageCount    = imageCount;
  swapchainCreateInfo.imageFormat      = surfaceFormat.format;
  swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
  swapchainCreateInfo.imageExtent      = mSwapchainExtent;
  swapchainCreateInfo.imageArrayLayers = 1; // the amount of layers each image consists of
  swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  uint32_t queueFamilyIndicesArray[] = {mQueueFamilyIndices.graphicsFamily, mQueueFamilyIndices.presentFamily};

  if (mQueueFamilyIndices.graphicsFamily != mQueueFamilyIndices.presentFamily) {
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

  VkResult result = vkCreateSwapchainKHR(mDevice, &swapchainCreateInfo, nullptr, &mSwapchain);
  logger::checkStep("vkCreateSwapchainKHR", result);

  // obtain the actual number of swapchain images
  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
  mSwapchainImages.resize(imageCount);
  mSwapchainImageViews.reserve(imageCount);
  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data());

  for (size_t i = 0; i < imageCount; i++) {
    mSwapchainImageViews.emplace_back(
        Image::createImageView(mDevice, mSwapchainImages[i], mSwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT));
  }
}

void VulkanApplicationContext::createAllocator() {
  // load vulkan functions dynamically
  VmaVulkanFunctions vmaVulkanFunc{};
  vmaVulkanFunc.vkAllocateMemory                        = vkAllocateMemory;
  vmaVulkanFunc.vkBindBufferMemory                      = vkBindBufferMemory;
  vmaVulkanFunc.vkBindImageMemory                       = vkBindImageMemory;
  vmaVulkanFunc.vkCreateBuffer                          = vkCreateBuffer;
  vmaVulkanFunc.vkCreateImage                           = vkCreateImage;
  vmaVulkanFunc.vkDestroyBuffer                         = vkDestroyBuffer;
  vmaVulkanFunc.vkDestroyImage                          = vkDestroyImage;
  vmaVulkanFunc.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
  vmaVulkanFunc.vkFreeMemory                            = vkFreeMemory;
  vmaVulkanFunc.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
  vmaVulkanFunc.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
  vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
  vmaVulkanFunc.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
  vmaVulkanFunc.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
  vmaVulkanFunc.vkMapMemory                             = vkMapMemory;
  vmaVulkanFunc.vkUnmapMemory                           = vkUnmapMemory;
  vmaVulkanFunc.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
  vmaVulkanFunc.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2;
  vmaVulkanFunc.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2;
  vmaVulkanFunc.vkBindBufferMemory2KHR                  = vkBindBufferMemory2;
  vmaVulkanFunc.vkBindImageMemory2KHR                   = vkBindImageMemory2;
  vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
  vmaVulkanFunc.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
  vmaVulkanFunc.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_2;
  allocatorInfo.physicalDevice         = mPhysicalDevice;
  allocatorInfo.device                 = mDevice;
  allocatorInfo.instance               = mVkInstance;
  allocatorInfo.pVulkanFunctions       = &vmaVulkanFunc;

  VkResult result = vmaCreateAllocator(&allocatorInfo, &mAllocator);
  logger::checkStep("vmaCreateAllocator", result);
}

// create a command pool for rendering commands and a command pool for gui commands (imgui)
void VulkanApplicationContext::createCommandPool() {
  VkCommandPoolCreateInfo commandPoolCreateInfo1{};
  commandPoolCreateInfo1.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo1.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily;

  VkResult result = vkCreateCommandPool(mDevice, &commandPoolCreateInfo1, nullptr, &mCommandPool);
  logger::checkStep("vkCreateCommandPool(commandPoolCreateInfo1)", result);

  VkCommandPoolCreateInfo commandPoolCreateInfo2{};
  commandPoolCreateInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo2.flags =
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allows the use of vkResetCommandBuffer
  commandPoolCreateInfo2.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily;

  result = vkCreateCommandPool(mDevice, &commandPoolCreateInfo2, nullptr, &mGuiCommandPool);
  logger::checkStep("vkCreateCommandPool(commandPoolCreateInfo2)", result);
}

VkFormat VulkanApplicationContext::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                                       VkFormatFeatureFlags features) const {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    }
    if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  logger::throwError("failed to find supported format!");
  return VK_FORMAT_UNDEFINED;
}