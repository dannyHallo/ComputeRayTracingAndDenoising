#pragma once

#include "utils/vulkan.h"
#include "vk_mem_alloc.h"
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

  const VkDevice &getDevice() const;

  const VkPhysicalDevice &getPhysicalDevice() const;

  const VkQueue &getGraphicsQueue() const;

  const VkQueue &getPresentQueue() const;

  const VkCommandPool &getCommandPool() const;

  const VmaAllocator &getAllocator() const;

  const vkb::Device &getVkbDevice() const;

  GLFWwindow *getWindow() const;

private:
  void initWindow(uint8_t windowSize);

  void createSurface();

  void createInstance();

  void createDevice();

  void createCommandPool();

private:
  GLFWwindow *mWindow;
  GLFWmonitor *mMonitor;
  vkb::Instance mVkbInstance;
  VkSurfaceKHR mSurface;
  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;
  VkCommandPool mCommandPool;
  VmaAllocator mAllocator;
  vkb::Device mVkbDevice;
};

namespace VulkanGlobal {
extern const VulkanApplicationContext context;
}
