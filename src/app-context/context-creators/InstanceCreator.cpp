#include "InstanceCreator.h"

#include "utils/Logger.h"

#include <set>

namespace {
// we can change the color of the debug messages from this callback function!
// in this case, we change the debug messages to red
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
                                             VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                             void * /*pUserData*/) {

  // we may change display color according to its importance level
  // if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){...}

  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT getDebugMessagerCreateInfo() {
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

  // Avoid some of the debug details by leaving some of the flags
  debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  // customize message severity here, to focus on the most significant messages that the validation layer can give us
  debugCreateInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  // leaving the following message severities out, for simpler validation debug infos
  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT

  // we'd like to leave all the message types out
  debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugCreateInfo.pfnUserCallback = debugCallback;
  return debugCreateInfo;
}

// setup runtime debug messager
void setupDebugMessager(VkInstance instance, VkDebugUtilsMessengerEXT &debugMessager) {
  auto createInfo = getDebugMessagerCreateInfo();

  assert(vkCreateDebugUtilsMessengerEXT != VK_NULL_HANDLE &&
         "vkCreateDebugUtilsMessengerEXT is a null function, call volkLoadInstance first");
  VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessager);
  Logger::checkStep("vkCreateDebugUtilsMessengerEXT", result);
}

// returns instance required extension names (i.e glfw, validation layers), they are device-irrational extensions
std::vector<const char *> getRequiredInstanceExtensions() {
  // Get glfw required extensions
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions = nullptr;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  // Due to the nature of the Vulkan interface, there is very little error information available to the developer and
  // application. By using the VK_EXT_debug_utils extension, developers can obtain more information. When combined with
  // validation layers, even more detailed feedback on the application’s use of Vulkan will be provided.
#ifndef NVALIDATIONLAYERS
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // NDEBUG

  return extensions;
}

bool checkInstanceLayerSupport(const std::vector<const char *> &layers) {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  Logger::print("available instance layers", availableLayers.size());
  std::set<std::string> availableLayersSet{};
  for (const auto &layerProperty : availableLayers) {
    availableLayersSet.insert(static_cast<const char *>(layerProperty.layerName));
    Logger::print("\t", static_cast<const char *>(layerProperty.layerName));
  }

  Logger::print();
  Logger::print("using instance layers", layers.size());

  std::vector<std::string> unavailableLayerNames{};
  // for each validation layer, we check for its validity from the avaliable layer pool
  for (const auto &layerName : layers) {
    Logger::print("\t", layerName);
    if (availableLayersSet.find(layerName) == availableLayersSet.end()) {
      unavailableLayerNames.emplace_back(layerName);
    }
  }

  if (unavailableLayerNames.empty()) {
    Logger::print("\t\t");
    return true;
  }

  for (const auto &unavailableLayerName : unavailableLayerNames) {
    Logger::print("\t", unavailableLayerName.c_str());
  }
  Logger::print("\t\t");
  return false;
}

} // namespace

void InstanceCreator::create(VkInstance &instance, VkDebugUtilsMessengerEXT &debugMessager,
                             const VkApplicationInfo &appInfo, const std::vector<const char *> &layers) {
  VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

  createInfo.pApplicationInfo = &appInfo;

  // Get all available extensions
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availavleExtensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availavleExtensions.data());

  // Logger::prints all available instance extensions
  Logger::print("available instance extensions", availavleExtensions.size());
  for (const auto &extension : availavleExtensions) {
    Logger::print("\t", static_cast<const char *>(extension.extensionName));
  }
  Logger::print();

  // get glfw (+ debug) extensions
  auto instanceRequiredExtensions    = getRequiredInstanceExtensions();
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(instanceRequiredExtensions.size());
  createInfo.ppEnabledExtensionNames = instanceRequiredExtensions.data();

  Logger::print("using instance extensions", instanceRequiredExtensions.size());
  for (const auto &extension : instanceRequiredExtensions) {
    Logger::print("\t", extension);
  }
  Logger::print();
  Logger::print();

#ifndef NVALIDATIONLAYERS
  // Setup debug messager info during vkCreateInstance and vkDestroyInstance

  if (!checkInstanceLayerSupport(layers)) {
    Logger::throwError("Validation layers requested, but not available!");
  }

  createInfo.enabledLayerCount   = static_cast<uint32_t>(layers.size());
  createInfo.ppEnabledLayerNames = layers.data();
  auto debugMessagerCreateInfo   = getDebugMessagerCreateInfo();
  createInfo.pNext               = &debugMessagerCreateInfo;
#else
  createInfo.enabledLayerCount = 0;
  createInfo.pNext             = nullptr;
#endif // NDEBUG

  // create VK Instance
  VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
  Logger::checkStep("vkCreateInstance", result);

  // load instance-related functions
  volkLoadInstance(instance);

#ifndef NVALIDATIONLAYERS
  setupDebugMessager(instance, debugMessager);
#endif // NDEBUG
}
