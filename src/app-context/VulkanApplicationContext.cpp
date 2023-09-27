#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

// VOLK_IMPLEMENTATION lets volk define the functions, by letting volk.h include
// volk.c this must only be defined in one translation unit
#define VOLK_IMPLEMENTATION
#include "app-context/VulkanApplicationContext.h"

#include "utils/Logger.h"

#include "memory/Image.h"

#include "context-creators/DeviceCreator.h"
#include "context-creators/InstanceCreator.h"
#include "context-creators/SurfaceCreator.h"
#include "context-creators/SwapchainCreator.h"

#include <algorithm>
#include <cstdint>
#include <set>

static const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
static const std::vector<const char *> requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

void VulkanApplicationContext::init(GLFWwindow *window) {
  Logger::print("Creating VulkanApplicationContext");
#ifndef NVALIDATIONLAYERS
  Logger::print("DEBUG mode is enabled");
#else
  Logger::print("DEBUG mode is disabled");
#endif // NDEBUG

  mGlWindow       = window;
  VkResult result = volkInitialize();
  Logger::checkStep("volkInitialize", result);

  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName   = "Compute Ray Tracing";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = "No Engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_2;
  InstanceCreator::create(mVkInstance, mDebugMessager, appInfo,
                          validationLayers);

  SurfaceCreator::create(mVkInstance, mSurface, mGlWindow);

  // selects physical device, creates logical device from that, decides queues,
  // loads device-related functions too
  DeviceCreator::create(mPhysicalDevice, mDevice, mQueueFamilyIndices,
                        mGraphicsQueue, mPresentQueue, mComputeQueue,
                        mTransferQueue, mVkInstance, mSurface,
                        requiredDeviceExtensions);

  SwapchainCreator::create(mSwapchain, mSwapchainImages, mSwapchainImageViews,
                           mSwapchainImageFormat, mSwapchainExtent, mSurface,
                           mDevice, mPhysicalDevice, mQueueFamilyIndices);

  createAllocator();
  createCommandPool();
}

VulkanApplicationContext *VulkanApplicationContext::getInstance() {
  static VulkanApplicationContext instance{};
  return &instance;
}

VulkanApplicationContext::VulkanApplicationContext() {}

VulkanApplicationContext::~VulkanApplicationContext() {
  Logger::print("Destroying VulkanApplicationContext");

  vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
  vkDestroyCommandPool(mDevice, mGuiCommandPool, nullptr);

  for (auto &swapchainImageView : mSwapchainImageViews) {
    vkDestroyImageView(mDevice, swapchainImageView, nullptr);
  }

  vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

  vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);

  // this step destroys allocated VkDestroyMemory allocated by VMA when creating
  // buffers and images, by destroying the global allocator
  vmaDestroyAllocator(mAllocator);
  vkDestroyDevice(mDevice, nullptr);

#ifndef NVALIDATIONLAYERS
  vkDestroyDebugUtilsMessengerEXT(mVkInstance, mDebugMessager, nullptr);
#endif // NDEBUG

  vkDestroyInstance(mVkInstance, nullptr);
}

void VulkanApplicationContext::createAllocator() {
  // load vulkan functions dynamically
  VmaVulkanFunctions vmaVulkanFunc{};
  vmaVulkanFunc.vkAllocateMemory              = vkAllocateMemory;
  vmaVulkanFunc.vkBindBufferMemory            = vkBindBufferMemory;
  vmaVulkanFunc.vkBindImageMemory             = vkBindImageMemory;
  vmaVulkanFunc.vkCreateBuffer                = vkCreateBuffer;
  vmaVulkanFunc.vkCreateImage                 = vkCreateImage;
  vmaVulkanFunc.vkDestroyBuffer               = vkDestroyBuffer;
  vmaVulkanFunc.vkDestroyImage                = vkDestroyImage;
  vmaVulkanFunc.vkFlushMappedMemoryRanges     = vkFlushMappedMemoryRanges;
  vmaVulkanFunc.vkFreeMemory                  = vkFreeMemory;
  vmaVulkanFunc.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
  vmaVulkanFunc.vkGetImageMemoryRequirements  = vkGetImageMemoryRequirements;
  vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties =
      vkGetPhysicalDeviceMemoryProperties;
  vmaVulkanFunc.vkGetPhysicalDeviceProperties  = vkGetPhysicalDeviceProperties;
  vmaVulkanFunc.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
  vmaVulkanFunc.vkMapMemory                    = vkMapMemory;
  vmaVulkanFunc.vkUnmapMemory                  = vkUnmapMemory;
  vmaVulkanFunc.vkCmdCopyBuffer                = vkCmdCopyBuffer;
  vmaVulkanFunc.vkGetBufferMemoryRequirements2KHR =
      vkGetBufferMemoryRequirements2;
  vmaVulkanFunc.vkGetImageMemoryRequirements2KHR =
      vkGetImageMemoryRequirements2;
  vmaVulkanFunc.vkBindBufferMemory2KHR = vkBindBufferMemory2;
  vmaVulkanFunc.vkBindImageMemory2KHR  = vkBindImageMemory2;
  vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties2KHR =
      vkGetPhysicalDeviceMemoryProperties2;
  vmaVulkanFunc.vkGetDeviceBufferMemoryRequirements =
      vkGetDeviceBufferMemoryRequirements;
  vmaVulkanFunc.vkGetDeviceImageMemoryRequirements =
      vkGetDeviceImageMemoryRequirements;

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_2;
  allocatorInfo.physicalDevice         = mPhysicalDevice;
  allocatorInfo.device                 = mDevice;
  allocatorInfo.instance               = mVkInstance;
  allocatorInfo.pVulkanFunctions       = &vmaVulkanFunc;

  VkResult result = vmaCreateAllocator(&allocatorInfo, &mAllocator);
  Logger::checkStep("vmaCreateAllocator", result);
}

// create a command pool for rendering commands and a command pool for gui
// commands (imgui)
void VulkanApplicationContext::createCommandPool() {
  VkCommandPoolCreateInfo commandPoolCreateInfo1{};
  commandPoolCreateInfo1.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo1.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily;

  VkResult result = vkCreateCommandPool(mDevice, &commandPoolCreateInfo1,
                                        nullptr, &mCommandPool);
  Logger::checkStep("vkCreateCommandPool(commandPoolCreateInfo1)", result);

  VkCommandPoolCreateInfo commandPoolCreateInfo2{};
  commandPoolCreateInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo2.flags =
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allows the use of
                                                       // vkResetCommandBuffer
  commandPoolCreateInfo2.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily;

  result = vkCreateCommandPool(mDevice, &commandPoolCreateInfo2, nullptr,
                               &mGuiCommandPool);
  Logger::checkStep("vkCreateCommandPool(commandPoolCreateInfo2)", result);
}
