#include "DeviceCreator.hpp"

#include "Common.hpp"
#include "utils/logger/Logger.hpp"

#include <set>
namespace {
bool _queueIndicesAreFilled(const ContextCreator::QueueFamilyIndices &indices) {
  return indices.computeFamily != -1 && indices.transferFamily != -1 &&
         indices.graphicsFamily != -1 && indices.presentFamily != -1;
}

bool _findQueueFamilies(ContextCreator::QueueFamilyIndices &indices,
                        const VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    const auto &queueFamily = queueFamilies[i];

    if (indices.computeFamily == -1) {
      if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
        indices.computeFamily = i;
      }
    }

    if (indices.transferFamily == -1) {
      if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) {
        indices.transferFamily = i;
      }
    }

    if (indices.graphicsFamily == -1) {
      if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
        uint32_t presentSupport = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport != 0) {
          indices.graphicsFamily = i;
          indices.presentFamily  = i;
        }
      }
    }

    if (_queueIndicesAreFilled(indices)) {
      return true;
    }
  }
  return false;
}

bool _checkDeviceExtensionSupport(Logger *logger, const VkPhysicalDevice &physicalDevice,
                                  const std::vector<const char *> &requiredDeviceExtensions) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> availableExtensionsSet{};
  for (const auto &extension : availableExtensions) {
    availableExtensionsSet.insert(static_cast<const char *>(extension.extensionName));
  }

  logger->info("available device extensions count: {}", availableExtensions.size());
  logger->println();
  logger->info("using device extensions: {}", requiredDeviceExtensions.size());
  for (const auto &extensionName : requiredDeviceExtensions) {
    logger->subInfo("{}", extensionName);
  }
  logger->println();
  logger->println();

  std::vector<std::string> unavailableExtensionNames{};
  for (const auto &requiredExtension : requiredDeviceExtensions) {
    if (availableExtensionsSet.find(requiredExtension) == availableExtensionsSet.end()) {
      unavailableExtensionNames.emplace_back(requiredExtension);
    }
  }

  if (unavailableExtensionNames.empty()) {
    return true;
  }

  logger->info("the following device extensions are not available:");
  for (const auto &unavailableExtensionName : unavailableExtensionNames) {
    logger->subInfo("{}", unavailableExtensionName);
  }
  return false;
}

// this function is also called in swapchain creation step
// so check if this overhead can be eliminated
// query for physical device's swapchain sepport details
ContextCreator::SwapchainSupportDetails querySwapchainSupport(VkSurfaceKHR surface,
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

void checkDeviceSuitable(Logger *logger, const VkSurfaceKHR &surface,
                         const VkPhysicalDevice &physicalDevice,
                         const std::vector<const char *> &requiredDeviceExtensions) {
  // Check if the queue family is valid
  ContextCreator::QueueFamilyIndices indices{};
  bool indicesAreFilled = _findQueueFamilies(indices, physicalDevice, surface);
  // Check extension support
  bool extensionSupported =
      _checkDeviceExtensionSupport(logger, physicalDevice, requiredDeviceExtensions);
  bool swapChainAdequate = false;
  if (extensionSupported) {
    ContextCreator::SwapchainSupportDetails swapChainSupport =
        querySwapchainSupport(surface, physicalDevice);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  // Query for device features if needed
  // VkPhysicalDeviceFeatures supportedFeatures;
  // vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
  if (indicesAreFilled && extensionSupported && swapChainAdequate) {
    return;
  }

  logger->error("physical device not suitable");
}

// helper function to customize the physical device ranking mechanism, returns
// the physical device with the highest score the marking criteria should be
// further optimized
VkPhysicalDevice selectBestDevice(Logger *logger,
                                  const std::vector<VkPhysicalDevice> &physicalDevices,
                                  const VkSurfaceKHR &surface,
                                  const std::vector<const char *> &requiredDeviceExtensions) {
  VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

  static constexpr uint32_t kDiscreteGpuMark   = 100;
  static constexpr uint32_t kIntegratedGpuMark = 20;

  // Give marks to all devices available, returns the best usable device
  std::vector<uint32_t> deviceMarks(physicalDevices.size());
  size_t deviceId = 0;

  logger->info("-----------------------------------------------------------------------");

  for (const auto &physicalDevice : physicalDevices) {

    VkPhysicalDeviceProperties deviceProperty;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperty);

    // discrete GPU will mark better
    if (deviceProperty.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      deviceMarks[deviceId] += kDiscreteGpuMark;
    } else if (deviceProperty.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      deviceMarks[deviceId] += kIntegratedGpuMark;
    }

    VkPhysicalDeviceMemoryProperties memoryProperty;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperty);

    auto *heapsPointer = static_cast<VkMemoryHeap *>(memoryProperty.memoryHeaps);
    auto heaps =
        std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + memoryProperty.memoryHeapCount);

    size_t deviceMemory = 0;
    for (const auto &heap : heaps) {
      // at least one heap has this flag
      if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0) {
        deviceMemory += heap.size;
      }
    }

    int constexpr kDesiredLengthOfDeviceName = 30;
    logger->info("[{}] {:<{}} memory {} mark {}", deviceId, deviceProperty.deviceName,
                 kDesiredLengthOfDeviceName, deviceMemory, deviceMarks[deviceId]);

    deviceId++;
  }

  logger->info("-----------------------------------------------------------------------");
  logger->println();

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
    logger->error("no suitable GPU found.");
  } else {
    VkPhysicalDeviceProperties bestDeviceProperty;
    vkGetPhysicalDeviceProperties(bestDevice, &bestDeviceProperty);
    logger->info("Selected: {}", static_cast<const char *>(bestDeviceProperty.deviceName));
    logger->println();

    checkDeviceSuitable(logger, surface, bestDevice, requiredDeviceExtensions);
  }
  return bestDevice;
}
} // namespace

// pick the most suitable physical device, and create logical device from it
void ContextCreator::createDevice(Logger *logger, VkPhysicalDevice &physicalDevice,
                                  VkDevice &device, QueueFamilyIndices &indices,
                                  QueueSelection &queueSelection, const VkInstance &instance,
                                  VkSurfaceKHR surface,
                                  const std::vector<const char *> &requiredDeviceExtensions) {
  // pick the physical device with the best performance
  {
    physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
      logger->error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    physicalDevice = selectBestDevice(logger, physicalDevices, surface, requiredDeviceExtensions);
  }

  // create logical device from the physical device we've picked
  {
    _findQueueFamilies(indices, physicalDevice, surface);

    std::set<uint32_t> queueFamilyIndicesSet = {indices.graphicsFamily, indices.presentFamily,
                                                indices.computeFamily, indices.transferFamily};

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.F; // ranges from 0 - 1.;
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
    // descriptorIndexing.pNext = &rayTracingStructure; // uncomment this to
    // enable the features above

    physicalDeviceFeatures.pNext = &descriptorIndexing;

    vkGetPhysicalDeviceFeatures2(physicalDevice,
                                 &physicalDeviceFeatures); // enable all the features our GPU has

    VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.pNext                = &physicalDeviceFeatures;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();
    // createInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.pEnabledFeatures = nullptr;

    // enabling device extensions
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

    // The enabledLayerCount and ppEnabledLayerNames fields of
    // VkDeviceCreateInfo are ignored by up-to-date implementations.
    deviceCreateInfo.enabledLayerCount   = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;

    VkResult res = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    if (res != VK_SUCCESS) {
      logger->error("failed to create logical device!");
    }

    // reduce loading overhead by specifing only one device is used
    volkLoadDevice(device);

    queueSelection.graphicsQueueIndex = indices.graphicsFamily;
    queueSelection.presentQueueIndex  = indices.presentFamily;
    queueSelection.computeQueueIndex  = indices.computeFamily;
    queueSelection.transferQueueIndex = indices.transferFamily;

    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &queueSelection.graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &queueSelection.presentQueue);
    vkGetDeviceQueue(device, indices.computeFamily, 0, &queueSelection.computeQueue);
    vkGetDeviceQueue(device, indices.transferFamily, 0, &queueSelection.transferQueue);

    // // if raytracing support requested - let's get raytracing properties to
    // // know shader header size and max recursion
    // mRTProps =
    // {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    // VkPhysicalDeviceProperties2
    // devProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2}; devProps.pNext
    // = &mRTProps; devProps.properties = {};

    // vkGetPhysicalDeviceProperties2(physicalDevice, &devProps);
  }
}