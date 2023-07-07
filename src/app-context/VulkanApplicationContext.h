#pragma once

// use stl version of std::min(), std::max(), and ignore the macro function with the same name provided by windows.h
#define NOMINMAX

// this should be defined first for the definition of VK_VERSION_1_0, which is used in glfw3.h
#include "utils/vulkan.h"

// glfw3 will define APIENTRY if it is not defined yet
#include "GLFW/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
// we undefine this to solve confiction with systemLog

#include "memory/Image.h"
#include "utils/Window.h"
#include "vk_mem_alloc.h"

#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

class VulkanApplicationContext {
  struct QueueFamilyIndices {
    // the wrapper optional can call .has_value to determine whether this var is assigned or not.
    // in case that they don't overlap
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
    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images) Surface formats
    // (pixel format, color space) Available presentation modes

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  } swapchainSupportDetails;

  const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
  const bool enableDebug                           = false;
  const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

public:
  VulkanApplicationContext();
  ~VulkanApplicationContext();

  // find the indices of the queue families that support drawing commands and presentation respectively
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
  void checkDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
  SwapchainSupportDetails querySwapchainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
  VkPhysicalDevice selectBestDevice(std::vector<VkPhysicalDevice> physicalDevices);
  VkExtent2D getSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};

extern VulkanApplicationContext vulkanApplicationContext;