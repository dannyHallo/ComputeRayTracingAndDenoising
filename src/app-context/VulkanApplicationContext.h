#pragma once

// use stl version of std::min(), std::max(), and ignore the macro function with the same name
// provided by windows.h
#define NOMINMAX

// this should be defined first for the definition of VK_VERSION_1_0, which is used in glfw3.h
#include "utils/vulkan.h"

// glfw3 will define APIENTRY if it is not defined yet
#include "glfw/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
// we undefine this to solve conflict with systemLog

#include "window/Window.h"

#include "vk_mem_alloc.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// also, this class should be configed out of class
class VulkanApplicationContext {
  static std::unique_ptr<VulkanApplicationContext> sInstance;

  // stores the indices of the each queue family, they might not overlap
  struct QueueFamilyIndices {
    uint32_t graphicsFamily = -1;
    uint32_t presentFamily  = -1;
    uint32_t computeFamily  = -1;
    uint32_t transferFamily = -1;
  } mQueueFamilyIndices;

  struct SwapchainSupportDetails {
    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height
    // of images) Surface formats (pixel format, color space) Available presentation modes

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  } mSwapchainSupportDetails;

  GLFWwindow *mGlWindow = nullptr;

  VkInstance mVkInstance           = VK_NULL_HANDLE;
  VkSurfaceKHR mSurface            = VK_NULL_HANDLE;
  VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
  VkDevice mDevice                 = VK_NULL_HANDLE;
  VmaAllocator mAllocator          = VK_NULL_HANDLE;

  // These queues are implicitly cleaned up when the device is destroyed
  VkQueue mGraphicsQueue = VK_NULL_HANDLE;
  VkQueue mPresentQueue  = VK_NULL_HANDLE;
  VkQueue mComputeQueue  = VK_NULL_HANDLE;
  VkQueue mTransferQueue = VK_NULL_HANDLE;

  VkCommandPool mCommandPool    = VK_NULL_HANDLE;
  VkCommandPool mGuiCommandPool = VK_NULL_HANDLE;

  VkDebugUtilsMessengerEXT mDebugMessager = VK_NULL_HANDLE;

  VkSwapchainKHR mSwapchain      = VK_NULL_HANDLE;
  VkFormat mSwapchainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D mSwapchainExtent    = {0, 0};

  std::vector<VkImage> mSwapchainImages;
  std::vector<VkImageView> mSwapchainImageViews;

public:
  // use glwindow to init the instance, can be only called once
  static VulkanApplicationContext *initInstance(GLFWwindow *glWindow = nullptr);

  static void destroyInstance();

  // get the singleton instance, must be called after initInstance()
  static VulkanApplicationContext *getInstance();

  ~VulkanApplicationContext();

  // disable move and copy
  VulkanApplicationContext(const VulkanApplicationContext &)            = delete;
  VulkanApplicationContext &operator=(const VulkanApplicationContext &) = delete;
  VulkanApplicationContext(VulkanApplicationContext &&)                 = delete;
  VulkanApplicationContext &operator=(VulkanApplicationContext &&)      = delete;

  // find the indices of the queue families that support drawing commands and presentation
  // respectively
  [[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                             VkFormatFeatureFlags features) const;

  [[nodiscard]] inline const VkInstance &getVkInstance() const { return mVkInstance; }
  [[nodiscard]] inline const VkDevice &getDevice() const { return mDevice; }
  [[nodiscard]] inline const VkSurfaceKHR &getSurface() const { return mSurface; }
  [[nodiscard]] inline const VkPhysicalDevice &getPhysicalDevice() const { return mPhysicalDevice; }

  [[nodiscard]] inline const VkCommandPool &getCommandPool() const { return mCommandPool; }
  [[nodiscard]] inline const VkCommandPool &getGuiCommandPool() const { return mGuiCommandPool; }
  [[nodiscard]] inline const VmaAllocator &getAllocator() const { return mAllocator; }
  [[nodiscard]] inline const std::vector<VkImage> &getSwapchainImages() const { return mSwapchainImages; }
  [[nodiscard]] inline const std::vector<VkImageView> &getSwapchainImageViews() const { return mSwapchainImageViews; }
  [[nodiscard]] inline size_t getSwapchainSize() const { return mSwapchainImages.size(); }
  [[nodiscard]] inline const VkFormat &getSwapchainImageFormat() const { return mSwapchainImageFormat; }
  [[nodiscard]] inline const VkExtent2D &getSwapchainExtent() const { return mSwapchainExtent; }
  [[nodiscard]] inline uint32_t getSwapchainExtentWidth() const { return mSwapchainExtent.width; }
  [[nodiscard]] inline uint32_t getSwapchainExtentHeight() const { return mSwapchainExtent.height; }
  [[nodiscard]] inline const VkSwapchainKHR &getSwapchain() const { return mSwapchain; }

  [[nodiscard]] const VkQueue &getGraphicsQueue() const { return mGraphicsQueue; }
  [[nodiscard]] const VkQueue &getPresentQueue() const { return mPresentQueue; }
  [[nodiscard]] const VkQueue &getComputeQueue() const { return mComputeQueue; }
  [[nodiscard]] const VkQueue &getTransferQueue() const { return mTransferQueue; }

  [[nodiscard]] uint32_t getGraphicsFamilyIndex() const { return mQueueFamilyIndices.graphicsFamily; }
  [[nodiscard]] uint32_t getPresentFamilyIndex() const { return mQueueFamilyIndices.presentFamily; }
  [[nodiscard]] uint32_t getComputeFamilyIndex() const { return mQueueFamilyIndices.computeFamily; }
  [[nodiscard]] uint32_t getTransferFamilyIndex() const { return mQueueFamilyIndices.transferFamily; }

private:
  VulkanApplicationContext(GLFWwindow *glWindow);

  void initWindow(uint8_t windowSize);
  void createInstance();
  void setupDebugMessager();
  void createSurface();
  void createDevice();
  void createSwapchain();
  void createCommandPool();
  void createAllocator();

  static std::vector<const char *> getRequiredInstanceExtensions();
  void checkDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
  static bool checkValidationLayerSupport();
  static bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

  // find the indices of the queue families, return whether the indices are fully filled
  bool findQueueFamilies(VkPhysicalDevice physicalDevice, QueueFamilyIndices &indices);
  static bool queueIndicesAreFilled(const VulkanApplicationContext::QueueFamilyIndices &indices);

  static SwapchainSupportDetails querySwapchainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
  VkPhysicalDevice selectBestDevice(std::vector<VkPhysicalDevice> physicalDevices);
  static VkExtent2D getSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};