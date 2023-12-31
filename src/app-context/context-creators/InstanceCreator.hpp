#pragma once

#include "utils/incl/Vulkan.hpp"

// glfw3 will define APIENTRY if it is not defined yet
#include "GLFW/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
// we undefine this to solve conflict with systemLog

#include <vector>

class Logger;
namespace ContextCreator {
void createInstance(Logger *logger, VkInstance &instance, VkDebugUtilsMessengerEXT &debugMessager,
                    const VkApplicationInfo &appInfo, const std::vector<const char *> &layers);
} // namespace ContextCreator