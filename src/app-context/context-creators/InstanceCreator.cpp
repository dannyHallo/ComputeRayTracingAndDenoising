#include "InstanceCreator.hpp"

#include "utils/logger/Logger.hpp"

// glfw3 will define APIENTRY if it is not defined yet
#include "GLFW/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
// undefine this to solve conflicts with other headers

#include <cassert>

#ifndef NVALIDATIONLAYERS
#include <iostream>
#include <set>
namespace {
// we can change the color of the debug messages from this callback function!
// in this case, we change the debug messages to red
VKAPI_ATTR VkBool32 VKAPI_CALL
_debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
               VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void * /*pUserData*/) {

  // we may change display color according to its importance level
  // if (messageSeverity >=
  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){...}

  std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';
  return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT _getDebugMessagerCreateInfo() {
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

  // Avoid some of the debug details by leaving some of the flags
  debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  // customize message severity here, to focus on the most significant messages
  // that the validation layer can give us
  debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  // leaving the following message severities out, for simpler validation debug
  // infos VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT

  // we'd like to leave all the message types out
  debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugCreateInfo.pfnUserCallback = _debugCallback;
  return debugCreateInfo;
}

// setup runtime debug messager
void _setupDebugMessager(VkInstance instance, VkDebugUtilsMessengerEXT &debugMessager) {
  auto createInfo = _getDebugMessagerCreateInfo();

  assert(vkCreateDebugUtilsMessengerEXT != VK_NULL_HANDLE &&
         "vkCreateDebugUtilsMessengerEXT is a null function, call "
         "volkLoadInstance first");
  vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessager);
}

bool _checkInstanceLayerSupport(Logger *logger, const std::vector<const char *> &layers) {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  logger->info("available instance layers: {}", availableLayers.size());
  std::set<std::string> availableLayersSet{};
  for (const auto &layerProperty : availableLayers) {
    availableLayersSet.insert(static_cast<const char *>(layerProperty.layerName));
    logger->subInfo("{}", static_cast<const char *>(layerProperty.layerName));
  }
  logger->println();

  logger->info("using instance layers: {}", layers.size());

  std::vector<std::string> unavailableLayerNames{};
  // for each validation layer, we check for its validity from the avaliable
  // layer pool
  for (const auto &layerName : layers) {
    logger->subInfo("{}", layerName);
    if (availableLayersSet.find(layerName) == availableLayersSet.end()) {
      unavailableLayerNames.emplace_back(layerName);
    }
  }

  if (unavailableLayerNames.empty()) {
    logger->println();
    return true;
  }

  for (const auto &unavailableLayerName : unavailableLayerNames) {
    logger->subInfo("{}", unavailableLayerName);
  }
  logger->println();
  return false;
}
} // namespace
#endif // NVALIDATIONLAYERS

namespace {
// returns instance required extension names (i.e glfw, validation layers), they
// are device-irrational extensions
std::vector<const char *> _getRequiredInstanceExtensions() {
  // Get glfw required extensions
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions = nullptr;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef __APPLE__
  extensions.push_back("VK_KHR_portability_enumeration");
#endif

  // Due to the nature of the Vulkan interface, there is very little error
  // information available to the developer and application. By using the
  // VK_EXT_debug_utils extension, developers can obtain more information. When
  // combined with validation layers, even more detailed feedback on the
  // application’s use of Vulkan will be provided.
#ifndef NVALIDATIONLAYERS
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // NDEBUG

  return extensions;
}

} // namespace

void ContextCreator::createInstance(Logger *logger, VkInstance &instance,
                                    VkDebugUtilsMessengerEXT &debugMessager,
                                    const VkApplicationInfo &appInfo,
                                    const std::vector<const char *> &layers) {
  VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

  createInfo.pApplicationInfo = &appInfo;
#ifdef __APPLE__
  createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  // Get all available extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availavleExtensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availavleExtensions.data());

  // logger->infos all available instance extensions
  logger->info("available instance extensions {}", availavleExtensions.size());
  for (const auto &extension : availavleExtensions) {
    logger->subInfo("{}", static_cast<const char *>(extension.extensionName));
  }
  logger->println();

  // get glfw (+ debug) extensions
  auto instanceRequiredExtensions    = _getRequiredInstanceExtensions();
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(instanceRequiredExtensions.size());
  createInfo.ppEnabledExtensionNames = instanceRequiredExtensions.data();

  logger->info("using instance extensions", instanceRequiredExtensions.size());
  for (const auto &extension : instanceRequiredExtensions) {
    logger->subInfo("{}", extension);
  }
  logger->println();

#ifndef NVALIDATIONLAYERS
  // setup debug messager info during vkCreateInstance and vkDestroyInstance
  if (!_checkInstanceLayerSupport(logger, layers)) {
    logger->error("Validation layers requested, but not available!");
  }

  createInfo.enabledLayerCount   = static_cast<uint32_t>(layers.size());
  createInfo.ppEnabledLayerNames = layers.data();
  auto debugMessagerCreateInfo   = _getDebugMessagerCreateInfo();
  createInfo.pNext               = &debugMessagerCreateInfo;
#else
  createInfo.enabledLayerCount = 0;
  createInfo.pNext             = nullptr;
#endif // NDEBUG

  // create VK Instance
  VkResult res = vkCreateInstance(&createInfo, nullptr, &instance);
  if (res != VK_SUCCESS) {
    logger->error("failed to create instance");
  }

  // load instance-related functions
  volkLoadInstance(instance);

#ifndef NVALIDATIONLAYERS
  _setupDebugMessager(instance, debugMessager);
#endif // NVLIDATIONLAYERS
}
