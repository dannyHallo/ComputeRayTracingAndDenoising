#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

// VOLK_IMPLEMENTATION lets volk define the functions, by letting volk.h include
// volk.c this must only be defined in one translation unit
#define VOLK_IMPLEMENTATION
#include "app-context/VulkanApplicationContext.hpp"

#include "memory/Image.hpp"
#include "utils/logger/Logger.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <set>

static const std::vector<const char *> validationLayers         = {"VK_LAYER_KHRONOS_validation"};
static const std::vector<const char *> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

void VulkanApplicationContext::init(Logger *logger, GLFWwindow *window) {
  _logger = logger;
  _logger->print("Initiating VulkanApplicationContext");
#ifndef NVALIDATIONLAYERS
  _logger->print("DEBUG mode is enabled");
#else
  _logger->print("DEBUG mode is disabled");
#endif // NDEBUG

  _glWindow       = window;
  VkResult result = volkInitialize();
  assert(result == VK_SUCCESS && "Failed to initialize volk");

  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName   = "Compute Ray Tracing";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = "No Engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_2;
  ContextCreator::createInstance(_logger, _vkInstance, _debugMessager, appInfo, validationLayers);

  ContextCreator::createSurface(_logger, _vkInstance, _surface, _glWindow);

  // selects physical device, creates logical device from that, decides queues,
  // loads device-related functions too
  ContextCreator::createDevice(_logger, _physicalDevice, _device, _queueFamilyIndices,
                               _graphicsQueue, _presentQueue, _computeQueue, _transferQueue,
                               _vkInstance, _surface, requiredDeviceExtensions);

  createSwapchainDimensionRelatedResources();

  _createAllocator();
  _createCommandPool();
}

VulkanApplicationContext *VulkanApplicationContext::getInstance() {
  static VulkanApplicationContext instance{};
  return &instance;
}

void VulkanApplicationContext::cleanupSwapchainDimensionRelatedResources() {
  for (auto &swapchainImageView : _swapchainImageViews) {
    vkDestroyImageView(_device, swapchainImageView, nullptr);
  }

  vkDestroySwapchainKHR(_device, _swapchain, nullptr);
}

void VulkanApplicationContext::createSwapchainDimensionRelatedResources() {
  ContextCreator::createSwapchain(_logger, _swapchain, _swapchainImages, _swapchainImageViews,
                                  _swapchainImageFormat, _swapchainExtent, _surface, _device,
                                  _physicalDevice, _queueFamilyIndices);
}

VulkanApplicationContext::~VulkanApplicationContext() {
  _logger->print("Destroying VulkanApplicationContext");

  vkDestroyCommandPool(_device, _commandPool, nullptr);
  vkDestroyCommandPool(_device, _guiCommandPool, nullptr);

  for (auto &swapchainImageView : _swapchainImageViews) {
    vkDestroyImageView(_device, swapchainImageView, nullptr);
  }

  vkDestroySwapchainKHR(_device, _swapchain, nullptr);

  vkDestroySurfaceKHR(_vkInstance, _surface, nullptr);

  // this step destroys allocated VkDestroyMemory allocated by VMA when creating
  // buffers and images, by destroying the global allocator
  vmaDestroyAllocator(_allocator);
  vkDestroyDevice(_device, nullptr);

#ifndef NVALIDATIONLAYERS
  vkDestroyDebugUtilsMessengerEXT(_vkInstance, _debugMessager, nullptr);
#endif // NDEBUG

  vkDestroyInstance(_vkInstance, nullptr);
}

void VulkanApplicationContext::_createAllocator() {
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
  allocatorInfo.physicalDevice         = _physicalDevice;
  allocatorInfo.device                 = _device;
  allocatorInfo.instance               = _vkInstance;
  allocatorInfo.pVulkanFunctions       = &vmaVulkanFunc;

  VkResult result = vmaCreateAllocator(&allocatorInfo, &_allocator);
  assert(result == VK_SUCCESS && "failed to create allocator");
}

// create a command pool for rendering commands and a command pool for gui
// commands (imgui)
void VulkanApplicationContext::_createCommandPool() {
  VkCommandPoolCreateInfo commandPoolCreateInfo1{};
  commandPoolCreateInfo1.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo1.queueFamilyIndex = _queueFamilyIndices.graphicsFamily;

  VkResult result = vkCreateCommandPool(_device, &commandPoolCreateInfo1, nullptr, &_commandPool);
  assert(result == VK_SUCCESS && "failed to create command pool");

  VkCommandPoolCreateInfo commandPoolCreateInfo2{};
  commandPoolCreateInfo2.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo2.queueFamilyIndex = _queueFamilyIndices.graphicsFamily;
  // this flag allows the use of vkResetCommandBuffer
  commandPoolCreateInfo2.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  result = vkCreateCommandPool(_device, &commandPoolCreateInfo2, nullptr, &_guiCommandPool);
  assert(result == VK_SUCCESS && "failed to create command pool");
}
