#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include "../utils/systemLog.h"
#include "memory/Image.h"

#include "VulkanApplicationContext.h"
#include "volk.c"

#include <iostream>

#define CHECK_VK_ERROR(result, message)                                                                                          \
  {                                                                                                                              \
    if (result != VK_SUCCESS) {                                                                                                  \
      assert(false && message);                                                                                                  \
    }                                                                                                                            \
  }

void showCursor(GLFWwindow *window) { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }

void hideCursor(GLFWwindow *window) {
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  if (glfwRawMouseMotionSupported()) {
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }
}

VulkanApplicationContext::VulkanApplicationContext() {
  volkInitialize();

  initWindow(WINDOW_SIZE_MAXIMAZED);
  createInstance();

  volkLoadInstance(mInstance);

  setupDebugMessager();
  createSurface();
  createDevice();

  volkLoadDevice(mDevice);

  createSwapchain();

  createAllocator();
  createCommandPool();
}

VulkanApplicationContext::~VulkanApplicationContext() {
  print("Cleaning up...");

  vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
  vmaDestroyAllocator(mAllocator);
  vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

  for (size_t i = 0; i < mSwapchainImageViews.size(); i++)
    vkDestroyImageView(mDevice, mSwapchainImageViews[i], nullptr);
  mSwapchainImageViews.clear();

  vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

  vkDestroyDevice(mDevice, nullptr);
  mDevice = nullptr;

  if (enableDebug)
    vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessager, nullptr);

  vkDestroyInstance(mInstance, nullptr);
  glfwDestroyWindow(mWindow);
}

void VulkanApplicationContext::initWindow(uint8_t windowSize) {
  glfwInit();

  mMonitor                = glfwGetPrimaryMonitor();    // Get primary monitor for future maximize function
  const GLFWvidmode *mode = glfwGetVideoMode(mMonitor); // May be used to change mode for this program

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Only OpenGL Api is supported, so no API here

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);       // Adapt colors (not needed)
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate); // Adapt framerate

  // Set window size
  switch (windowSize) {
  case WINDOW_SIZE_FULLSCREEN:
    mWindow = glfwCreateWindow(mode->width, mode->height, "Loading...", mMonitor, nullptr);
    break;
  case WINDOW_SIZE_MAXIMAZED:
    mWindow = glfwCreateWindow(mode->width, mode->height, "Loading...", nullptr, nullptr);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    break;
  case WINDOW_SIZE_HOVER:
    mWindow = glfwCreateWindow(WIDTH, HEIGHT, "Loading...", nullptr, nullptr);
    break;
  }

  hideCursor(mWindow);
  // window callbacks are in main.cpp
}

bool VulkanApplicationContext::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  // print all availiable layers
  print("available validation layers", availableLayers.size());
  for (const auto &layerProperty : availableLayers)
    print("\t", layerProperty.layerName);
  print();

  // for each validation layer, we check for its validity from the avaliable layer pool
  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound)
      return false;
  }
  return true;
}

// returns instance required extension names (i.e glfw, validation layers), they are device-irrational extensions
std::vector<const char *> VulkanApplicationContext::getRequiredInstanceExtensions() {
  // Get glfw required extensions
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  // Due to the nature of the Vulkan interface, there is very little error information available to the developer and application.
  // By using the VK_EXT_debug_utils extension, developers can obtain more information. When combined with validation layers, even
  // more detailed feedback on the application’s use of Vulkan will be provided.
  if (enableDebug)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}

// we can change the color of the debug messages from this callback function!
// in this case, we change the debug messages to red
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  // we may change display color according to its importance level
  // if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)

  return VK_FALSE;
}

void populateDebugMessagerInfo(VkDebugUtilsMessengerCreateInfoEXT &debugCreateInfo) {
  // Avoid some of the debug details by leaving some of the flags
  debugCreateInfo       = {};
  debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  // customize message severity here, to focus on the most significant messages that the validation layer can give us
  debugCreateInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  // leaving the following message severities out, for simpler validation debug infos
  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT

  // we'd like to leave all the message types out
  debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugCreateInfo.pfnUserCallback = debugCallback;
}

void VulkanApplicationContext::createInstance() {
  VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

  if (enableDebug && !checkValidationLayerSupport())
    throw std::runtime_error("Validation layers requested, but not available!");

  VkApplicationInfo appInfo{};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "Compute Ray Tracing";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // dev-supplied
  appInfo.pEngineName        = "No Engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0); // dev-supplied
  appInfo.apiVersion         = VK_API_VERSION_1_2;

  createInfo.pApplicationInfo = &appInfo;

  // Get all available extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availavleExtensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availavleExtensions.data());

  // prints all available instance extensions
  print("available instance extensions", availavleExtensions.size());
  for (const auto &extension : availavleExtensions)
    print("\t", extension.extensionName);
  print();

  // get glfw (+ debug) extensions
  auto instanceRequiredExtensions    = getRequiredInstanceExtensions();
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(instanceRequiredExtensions.size());
  createInfo.ppEnabledExtensionNames = instanceRequiredExtensions.data();

  print("enabled extensions", instanceRequiredExtensions.size());
  for (const auto &extension : instanceRequiredExtensions)
    std::cout << "\t" << extension << std::endl;
  print();

  // Setup debug messager info during vkCreateInstance and vkDestroyInstance
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (enableDebug) {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    populateDebugMessagerInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext             = nullptr;
  }

  // Create VK Instance
  if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
    throw std::runtime_error("failed to create instance!");
}

// setup runtime debug messager
void VulkanApplicationContext::setupDebugMessager() {
  if (!enableDebug)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessagerInfo(createInfo);

  if (vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessager) != VK_SUCCESS)
    throw std::runtime_error("failed to setup debug messager!");
}

void VulkanApplicationContext::createSurface() {
  if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
}

VkSampleCountFlagBits getDeviceMaxUsableSampleCount(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

  VkSampleCountFlags counts =
      physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }

  return VK_SAMPLE_COUNT_1_BIT;
}

bool VulkanApplicationContext::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

  print("available device extensions", availableExtensions.size());
  for (VkExtensionProperties extensionProperty : availableExtensions) {
    print("\t", extensionProperty.extensionName);
  }

  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  print();

  if (!requiredExtensions.empty()) {
    print("the following extension requirement is not met:");
    for (std::string extensionNotMetName : requiredExtensions) {
      print("\t", extensionNotMetName.c_str());
    }
    return false;
  }
  return true;
}

bool VulkanApplicationContext::checkDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice) {
  // Check if the queue family is valid
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  // Check extension support
  bool extensionSupported = checkDeviceExtensionSupport(physicalDevice);
  bool swapChainAdequate  = false;
  if (extensionSupported) {
    SwapchainSupportDetails swapChainSupport = querySwapchainSupport(surface, physicalDevice);
    swapChainAdequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  // Query for device features if needed
  // VkPhysicalDeviceFeatures supportedFeatures;
  // vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
  if (indices.isComplete() && extensionSupported && swapChainAdequate) {
    return true;
  }
  throw std::runtime_error("device is not suitable!");
  return false;
}

// helper function to customize the physical device ranking mechanism, returns the physical device with the highest score
VkPhysicalDevice VulkanApplicationContext::selectBestDevice(std::vector<VkPhysicalDevice> physicalDevices) {
  VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

  // Give marks to all devices available, returns the best usable device
  std::vector<uint32_t> deviceMarks(physicalDevices.size());
  size_t deviceId = 0;

  print("-------------------------------------------------------");

  for (const auto &physicalDevice : physicalDevices) {

    VkPhysicalDeviceProperties deviceProperty;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperty);

    // Discrete GPU will mark better
    if (deviceProperty.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      deviceMarks[deviceId] += 100;
    } else if (deviceProperty.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      deviceMarks[deviceId] += 20;
    }

    VkPhysicalDeviceMemoryProperties memoryProperty;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperty);

    auto heapsPointer = memoryProperty.memoryHeaps;
    auto heaps        = std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + memoryProperty.memoryHeapCount);

    size_t deviceMemory = 0;
    for (const auto &heap : heaps) {
      // At least one heap has this flag
      if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
        deviceMemory += heap.size;
      }
    }

    // MSAA
    VkSampleCountFlagBits msaaSamples = getDeviceMaxUsableSampleCount(physicalDevice);

    std::cout << "Device " << deviceId << "    " << deviceProperty.deviceName << "    Memory in bytes: " << deviceMemory
              << "    MSAA max sample count: " << msaaSamples << "    Mark: " << deviceMarks[deviceId] << "\n";

    deviceId++;
  }

  print("-------------------------------------------------------");
  print();

  uint32_t bestMark = 0;
  deviceId          = 0;

  for (const auto &deviceMark : deviceMarks) {
    if (deviceMark > bestMark) {
      bestMark   = deviceMark;
      bestDevice = physicalDevices[deviceId];
    }

    deviceId++;
  }

  if (bestDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  } else {
    VkPhysicalDeviceProperties bestDeviceProperty;
    vkGetPhysicalDeviceProperties(bestDevice, &bestDeviceProperty);
    std::cout << "Selected: " << bestDeviceProperty.deviceName << std::endl;
    print();

    checkDeviceSuitable(mSurface, bestDevice);
  }
  return bestDevice;
}

VulkanApplicationContext::QueueFamilyIndices VulkanApplicationContext::findQueueFamilies(VkPhysicalDevice physicalDevice) {
  // find required queue families, distribute every queue family uniformly if possible
  QueueFamilyIndices indices;

  // query for all queue families
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

  const VkQueueFlagBits requiredFlags[3] = {VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT};
  for (uint8_t i = 0; i < 3; i++) {
    VkQueueFlagBits flag = requiredFlags[i];

    if (flag == VK_QUEUE_COMPUTE_BIT) {
      // assign compute queue index with the queue that supports compute functionality
      for (uint32_t j = 0; j < queueFamilyCount; ++j) {
        if ((queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
          indices.computeFamily = j;
          break;
        }
      }
      // otherwise, assign compute queue to the first queue that supports compute functionality
      if (!indices.computeFamily.has_value()) {
        for (uint32_t j = 0; j < queueFamilyCount; ++j) {
          if (queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = j;
            break;
          }
        }
      }
    }

    else if (flag == VK_QUEUE_TRANSFER_BIT) {
      for (uint32_t j = 0; j < queueFamilyCount; ++j) {
        if ((queueFamilies[j].queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
          indices.transferFamily = j;
          break;
        }
      }
      if (!indices.transferFamily.has_value()) {
        for (uint32_t j = 0; j < queueFamilyCount; ++j) {
          if (queueFamilies[j].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = j;
            break;
          }
        }
      }
    }

    // Graphics & Present
    else {
      for (uint32_t j = 0; j < queueFamilyCount; ++j) {
        if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          indices.graphicsFamily = j;
          break;
        }
      }

      for (uint32_t j = 0; j < queueFamilyCount; ++j) {
        // See if the current queue family supports KHR
        // extension (for presenting use)
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, j, mSurface, &presentSupport);

        if (presentSupport) {
          indices.presentFamily = j;
          break;
        }
      }
    }
  }
  return indices;
}

// pick the most suitable physical device, and create logical device from it
void VulkanApplicationContext::createDevice() {
  // pick the physical device with the best performance
  {
    mPhysicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
      throw std::runtime_error("failed to find a single GPU with Vulkan support!");

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, physicalDevices.data());

    mPhysicalDevice = selectBestDevice(physicalDevices);
  }

  // create logical device from the physical device we've picked
  {
    queueFamilyIndices = findQueueFamilies(mPhysicalDevice);

    std::set<uint32_t> queueFamilyIndicesSet = {
        queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value(),
        queueFamilyIndices.computeFamily.value(), queueFamilyIndices.transferFamily.value()};

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f; // ranges from 0 - 1.;
    for (uint32_t queueFamilyIndex : queueFamilyIndicesSet) {
      VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
      queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
      queueCreateInfo.queueCount       = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddress = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
    bufferDeviceAddress.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipeline = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rayTracingPipeline.pNext              = &bufferDeviceAddress;
    rayTracingPipeline.rayTracingPipeline = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR rayTracingStructure = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    rayTracingStructure.pNext                 = &rayTracingPipeline;
    rayTracingStructure.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexing = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
    descriptorIndexing.pNext = &rayTracingStructure;

    physicalDeviceFeatures.pNext = &descriptorIndexing;

    vkGetPhysicalDeviceFeatures2(mPhysicalDevice, &physicalDeviceFeatures); // enable all the features our GPU has

    VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.pNext                = &physicalDeviceFeatures;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();
    // createInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.pEnabledFeatures = nullptr;

    // enabling device extensions
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // The enabledLayerCount and ppEnabledLayerNames fields of
    // VkDeviceCreateInfo are ignored by up-to-date implementations.
    deviceCreateInfo.enabledLayerCount   = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;

    if (vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice) != VK_SUCCESS)
      throw std::runtime_error("failed to create logical device!");

    vkGetDeviceQueue(mDevice, queueFamilyIndices.graphicsFamily.value(), 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, queueFamilyIndices.presentFamily.value(), 0, &mPresentQueue);
    vkGetDeviceQueue(mDevice, queueFamilyIndices.computeFamily.value(), 0, &mComputeQueue);
    vkGetDeviceQueue(mDevice, queueFamilyIndices.transferFamily.value(), 0, &mTransferQueue);

    // // if raytracing support requested - let's get raytracing properties to
    // // know shader header size and max recursion
    // mRTProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    // VkPhysicalDeviceProperties2 devProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    // devProps.pNext      = &mRTProps;
    // devProps.properties = {};

    // vkGetPhysicalDeviceProperties2(mPhysicalDevice, &devProps);
  }
}

// query for physical device's swapchain sepport details
VulkanApplicationContext::SwapchainSupportDetails
VulkanApplicationContext::querySwapchainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice) {
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
  uint32_t presentModeCount;
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
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
      return availableFormat;
  }

  print("Surface format requirement didn't meet, the first available format is chosen!");
  return availableFormats[0];
}

// choose the present mode of the swapchain
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
  // our preferance: Mailbox present mode
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      return availablePresentMode;
  }

  print("Present mode preferance doesn't meet, switching to FIFO");
  return VK_PRESENT_MODE_FIFO_KHR;
}

// return the current extent, or create another one
VkExtent2D VulkanApplicationContext::getSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  // if the current extent is valid (we can read the width and height out of it)
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    std::cout << "Using resolution: (" << capabilities.currentExtent.width << ", " << capabilities.currentExtent.height << ")"
              << std::endl;
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(mWindow, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
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
  print("number of swapchain images", imageCount);

  VkSwapchainCreateInfoKHR swapchainCreateInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainCreateInfo.surface          = mSurface;
  swapchainCreateInfo.minImageCount    = imageCount;
  swapchainCreateInfo.imageFormat      = surfaceFormat.format;
  swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
  swapchainCreateInfo.imageExtent      = mSwapchainExtent;
  swapchainCreateInfo.imageArrayLayers = 1; // the amount of layers each image consists of
  swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  uint32_t queueFamilyIndicesArray[] = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};

  if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
    // images can be used across multiple queue families without explicit ownership transfers.
    swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    swapchainCreateInfo.queueFamilyIndexCount = 2;
    swapchainCreateInfo.pQueueFamilyIndices   = queueFamilyIndicesArray;
  } else {
    // an image is owned by one queue family at a time and ownership must be explicitly transferred before the image is being used
    // in another queue family. This offers the best performance.
    swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;       // Optional
    swapchainCreateInfo.pQueueFamilyIndices   = nullptr; // Optional
  }

  swapchainCreateInfo.preTransform   = swapchainSupport.capabilities.currentTransform;
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  swapchainCreateInfo.presentMode = presentMode;
  swapchainCreateInfo.clipped     = VK_TRUE;

  // window resize logic : null
  swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(mDevice, &swapchainCreateInfo, nullptr, &mSwapchain) != VK_SUCCESS)
    throw std::runtime_error("failed to create swap chain!");

  // Get swap chain images from the swap chain created
  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
  mSwapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data());

  for (size_t i = 0; i < imageCount; i++) {
    mSwapchainImageViews.emplace_back(
        ImageUtils::createImageView(mSwapchainImages[i], mSwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1));
  }
}

void VulkanApplicationContext::createAllocator() {
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
  allocatorInfo.instance               = mInstance;
  allocatorInfo.pVulkanFunctions       = &vmaVulkanFunc;

  if (vmaCreateAllocator(&allocatorInfo, &mAllocator) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command memory allocator!");
  }
}

void VulkanApplicationContext::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

VkFormat VulkanApplicationContext::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                                       VkFormatFeatureFlags features) const {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}