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
#include <optional>
#include <set>
#include <string>
#include <vector>

class VulkanApplicationContext {
  // stores the indices of the each queue family, they might not overlap
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;

    void print();

    [[nodiscard]] bool isComplete() const {
      return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value() &&
             presentFamily.has_value();
    }
  } queueFamilyIndices;

  struct SwapchainSupportDetails {
    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height
    // of images) Surface formats (pixel format, color space) Available presentation modes

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  } swapchainSupportDetails;

  std::unique_ptr<Window> mWindow  = nullptr;
  VkInstance mInstance             = VK_NULL_HANDLE;
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
  VkExtent2D mSwapchainExtent{};

  std::vector<VkImage> mSwapchainImages;
  std::vector<VkImageView> mSwapchainImageViews;

public:
  VulkanApplicationContext();
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

  [[nodiscard]] inline const VkInstance &getInstance() const { return mInstance; }
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

  [[nodiscard]] uint32_t getGraphicsFamilyIndex() const { return queueFamilyIndices.graphicsFamily.value(); }
  [[nodiscard]] uint32_t getPresentFamilyIndex() const { return queueFamilyIndices.presentFamily.value(); }
  [[nodiscard]] uint32_t getComputeFamilyIndex() const { return queueFamilyIndices.computeFamily.value(); }
  [[nodiscard]] uint32_t getTransferFamilyIndex() const { return queueFamilyIndices.transferFamily.value(); }

  Window &getWindowClass() { return *mWindow; }
  [[nodiscard]] GLFWwindow *getWindow() const { return mWindow->getWindow(); }
  [[nodiscard]] GLFWmonitor *getMonitor() const { return mWindow->getMonitor(); }

private:
  void initWindow(uint8_t windowSize);
  void createInstance();
  void setupDebugMessager();
  void createSurface();
  void createDevice();
  void createSwapchain();
  void createCommandPool();
  void createAllocator();

  static bool checkValidationLayerSupport();
  static std::vector<const char *> getRequiredInstanceExtensions();
  void checkDeviceSuitable(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
  bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
  SwapchainSupportDetails querySwapchainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
  VkPhysicalDevice selectBestDevice(std::vector<VkPhysicalDevice> physicalDevices);
  VkExtent2D getSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};

extern VulkanApplicationContext vulkanApplicationContext;