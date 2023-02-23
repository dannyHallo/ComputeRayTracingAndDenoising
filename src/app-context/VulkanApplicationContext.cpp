#include <iostream>
#define VMA_IMPLEMENTATION
#include "VulkanApplicationContext.h"

void showCursor(GLFWwindow *window) { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }

void hideCursor(GLFWwindow *window) {
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  if (glfwRawMouseMotionSupported()) {
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }
}

VulkanApplicationContext::VulkanApplicationContext() {
  initWindow(WINDOW_SIZE_MAXIMAZED);
  createInstance();
  createSurface();
  createDevice();

  createCommandPool();
}

VulkanApplicationContext::~VulkanApplicationContext() {
  std::cout << "Destroying context"
            << "\n";
  vkDestroyCommandPool(mVkbDevice.device, mCommandPool, nullptr);
  vmaDestroyAllocator(mAllocator);
  vkDestroySurfaceKHR(mVkbInstance.instance, mSurface, nullptr);

  vkb::destroy_device(mVkbDevice);
  vkb::destroy_instance(mVkbInstance);
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

void VulkanApplicationContext::createInstance() {
  vkb::InstanceBuilder instance_builder;
  auto instance_builder_return = instance_builder
                                     // Instance creation configuration
                                     .request_validation_layers()
                                     .use_default_debug_messenger()
                                     .enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
                                     .build();
  if (!instance_builder_return)
    throw std::runtime_error("Failed to create Vulkan instance. Error: " + instance_builder_return.error().message());

  mVkbInstance = instance_builder_return.value();
}

void VulkanApplicationContext::createSurface() {
  if (glfwCreateWindowSurface(mVkbInstance.instance, mWindow, nullptr, &mSurface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

void VulkanApplicationContext::createDevice() {
  vkb::PhysicalDeviceSelector phys_device_selector(mVkbInstance);
  auto phys_dev_ret =
      phys_device_selector.add_desired_extension("VK_KHR_portability_subset").set_surface(mSurface).select();
  if (!phys_dev_ret) {
    throw std::runtime_error("Failed to create physical device. Error: " + phys_dev_ret.error().message());
  }

  std::cout << "Selected device name: " << phys_dev_ret.value().properties.deviceName << std::endl;
  vkb::DeviceBuilder device_builder{phys_dev_ret.value()};
  auto dev_ret = device_builder.build();
  if (!dev_ret) {
    throw std::runtime_error("Failed to create device. Error: " + dev_ret.error().message());
  }
  mVkbDevice = dev_ret.value();

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_0;
  allocatorInfo.physicalDevice         = phys_dev_ret.value(); // implicit conversion inside physicalDevice
  allocatorInfo.device                 = mVkbDevice.device;
  allocatorInfo.instance               = mVkbInstance.instance;

  if (vmaCreateAllocator(&allocatorInfo, &mAllocator) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command memory allocator!");
  }
}

void VulkanApplicationContext::createCommandPool() {
  auto g_queue_ret = mVkbDevice.get_queue(vkb::QueueType::graphics);
  if (!g_queue_ret) {
    throw std::runtime_error("Failed to create graphics queue. Error: " + g_queue_ret.error().message());
  }
  m_graphicsQueue = g_queue_ret.value();

  auto p_queue_ret = mVkbDevice.get_queue(vkb::QueueType::present);
  if (!p_queue_ret) {
    throw std::runtime_error("Failed to create present queue. Error: " + p_queue_ret.error().message());
  }
  m_presentQueue = p_queue_ret.value();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = mVkbDevice.get_queue_index(vkb::QueueType::graphics).value();
  if (vkCreateCommandPool(mVkbDevice.device, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

VkFormat VulkanApplicationContext::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                                       VkFormatFeatureFlags features) const {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(mVkbDevice.physical_device, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}

const VkDevice &VulkanApplicationContext::getDevice() const { return mVkbDevice.device; }

const VkPhysicalDevice &VulkanApplicationContext::getPhysicalDevice() const { return mVkbDevice.physical_device; }

const VkQueue &VulkanApplicationContext::getGraphicsQueue() const { return m_graphicsQueue; }

const VkQueue &VulkanApplicationContext::getPresentQueue() const { return m_presentQueue; }

const VkCommandPool &VulkanApplicationContext::getCommandPool() const { return mCommandPool; }

const VmaAllocator &VulkanApplicationContext::getAllocator() const { return mAllocator; }

const vkb::Device &VulkanApplicationContext::getVkbDevice() const { return mVkbDevice; }

GLFWwindow *VulkanApplicationContext::getWindow() const { return mWindow; }
