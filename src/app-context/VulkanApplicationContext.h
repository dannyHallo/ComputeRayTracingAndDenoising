#pragma once

#define VK_NO_PROTOTYPES // required in dynamic linking
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX

#include "utils/vulkan.h"

#include "GLFW/glfw3.h"
#include "memory/Image.h"
#include "vk_mem_alloc.h"

#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

const uint32_t WIDTH  = 1280;
const uint32_t HEIGHT = 1280;

#define WINDOW_SIZE_FULLSCREEN 0
#define WINDOW_SIZE_MAXIMAZED 1
#define WINDOW_SIZE_HOVER 2

class VulkanApplicationContext {
public:
  VulkanApplicationContext();
  ~VulkanApplicationContext();

  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features) const;
  const VkDevice &getDevice() const { return mDevice; }
  const VkSurfaceKHR &getSurface() const { return mSurface; }
  const VkPhysicalDevice &getPhysicalDevice() const { return mPhysicalDevice; }
  const VkQueue &getGraphicsQueue() const { return mGraphicsQueue; }
  const VkQueue &getPresentQueue() const { return mPresentQueue; }
  const VkQueue &getComputeQueue() const { return mComputeQueue; }
  const VkQueue &getTransferQueue() const { return mTransferQueue; }
  const VkCommandPool &getCommandPool() const { return mCommandPool; }
  const VmaAllocator &getAllocator() const { return mAllocator; }
  const std::vector<VkImage> &getSwapchainImages() const { return mSwapchainImages; }
  const std::vector<VkImageView> &getSwapchainImageViews() const { return mSwapchainImageViews; }
  const VkFormat &getSwapchainImageFormat() const { return mSwapchainImageFormat; }
  const VkExtent2D &getSwapchainExtent() const { return mSwapchainExtent; }
  const VkSwapchainKHR &getSwapchain() const { return mSwapchain; }

  GLFWwindow *getWindow() const { return mWindow; }

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
  const bool enableDebug                           = true;
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

  GLFWwindow *mWindow;
  GLFWmonitor *mMonitor;
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

namespace VulkanGlobal {
extern const VulkanApplicationContext context;
}
