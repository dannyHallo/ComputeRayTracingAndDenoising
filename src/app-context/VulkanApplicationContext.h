#pragma once

#define NOMINMAX

#include "utils/vulkan.h"

#include "GLFW/glfw3.h"
#include "memory/Image.h"
#include "utils/Window.h"
#include "vk_mem_alloc.h"

#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

class VulkanApplicationContext {
public:
  VulkanApplicationContext();
  ~VulkanApplicationContext();

  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features) const;
  const VkInstance &getInstance() const { return mInstance; }
  const VkDevice &getDevice() const { return mDevice; }
  const VkSurfaceKHR &getSurface() const { return mSurface; }
  const VkPhysicalDevice &getPhysicalDevice() const { return mPhysicalDevice; }

  const VkCommandPool &getCommandPool() const { return mCommandPool; }
  const VkCommandPool &getGuiCommandPool() const { return mGuiCommandPool; }
  const VmaAllocator &getAllocator() const { return mAllocator; }
  const std::vector<VkImage> &getSwapchainImages() const { return mSwapchainImages; }
  const std::vector<VkImageView> &getSwapchainImageViews() const { return mSwapchainImageViews; }
  const VkFormat &getSwapchainImageFormat() const { return mSwapchainImageFormat; }
  const VkExtent2D &getSwapchainExtent() const { return mSwapchainExtent; }
  const VkSwapchainKHR &getSwapchain() const { return mSwapchain; }

  const VkQueue &getGraphicsQueue() const { return mGraphicsQueue; }
  const VkQueue &getPresentQueue() const { return mPresentQueue; }
  const VkQueue &getComputeQueue() const { return mComputeQueue; }
  const VkQueue &getTransferQueue() const { return mTransferQueue; }

  const uint32_t getGraphicsFamilyIndex() const { return queueFamilyIndices.graphicsFamily.value(); }
  const uint32_t getPresentFamilyIndex() const { return queueFamilyIndices.presentFamily.value(); }
  const uint32_t getComputeFamilyIndex() const { return queueFamilyIndices.computeFamily.value(); }
  const uint32_t getTransferFamilyIndex() const { return queueFamilyIndices.transferFamily.value(); }

  Window &getWindowClass() { return mWindow; }
  GLFWwindow *getWindow() const { return mWindow.getWindow(); }
  GLFWmonitor *getMonitor() const { return mWindow.getMonitor(); }

private:
  struct QueueFamilyIndices {
    // The wrapper optional can call .hasValue to determine
    // whether this var is assigned or not.

    // Tho they are mostly the same, it's actually possible that the queue
    // families supporting drawing commands and the ones supporting
    // presentation do not overlap.
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;

    void print() {
      std::cout << "Graphics Family: " << graphicsFamily.value() << std::endl;
      std::cout << "Persent Family: " << presentFamily.value() << std::endl;
      std::cout << "Compute Family: " << computeFamily.value() << std::endl;
      std::cout << "Transfer Family: " << transferFamily.value() << std::endl;
    }

    bool isComplete() {
      return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value() && presentFamily.has_value();
    }
  } queueFamilyIndices;

  struct SwapchainSupportDetails {
    // Basic surface capabilities (min/max number of images in swap chain,
    // min/max width and height of images) Surface formats (pixel format, color
    // space) Available presentation modes
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  } swapchainSupportDetails;

  const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
  const bool enableDebug                           = false;
  const std::vector<const char *> deviceExtensions = {
      // 0
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,

      // 1
      VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,

      // 2
      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_SPIRV_1_4_EXTENSION_NAME, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,

      // 3
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
      VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};

  Window mWindow;
  VkInstance mInstance;
  VkSurfaceKHR mSurface;
  VkPhysicalDevice mPhysicalDevice;
  VkDevice mDevice;
  VmaAllocator mAllocator;

  // These queues are implicitly cleaned up when the device is destroyed
  VkQueue mGraphicsQueue;
  VkQueue mPresentQueue;
  VkQueue mComputeQueue;
  VkQueue mTransferQueue;

  VkDebugUtilsMessengerEXT mDebugMessager;

  VkSwapchainKHR mSwapchain;
  VkFormat mSwapchainImageFormat;
  VkExtent2D mSwapchainExtent;
  std::vector<VkImage> mSwapchainImages;
  std::vector<VkImageView> mSwapchainImageViews;

  VkCommandPool mCommandPool;
  VkCommandPool mGuiCommandPool;

private:
  void initWindow(uint8_t windowSize);
  void createInstance();
  void setupDebugMessager();
  void createSurface();
  void createDevice();
  void createSwapchain();
  void createCommandPool();
  void createAllocator();

  bool checkValidationLayerSupport();
  std::vector<const char *> getRequiredInstanceExtensions();
  bool checkDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
  SwapchainSupportDetails querySwapchainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
  VkPhysicalDevice selectBestDevice(std::vector<VkPhysicalDevice> physicalDevices);
  VkExtent2D getSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};

extern VulkanApplicationContext vulkanApplicationContext;