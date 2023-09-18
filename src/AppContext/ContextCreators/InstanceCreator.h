#pragma once

#include "utils/vulkan.h"

// glfw3 will define APIENTRY if it is not defined yet
#include "glfw/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
// we undefine this to solve conflict with systemLog

#include <vector>

namespace InstanceCreator {
void create(VkInstance &instance, VkDebugUtilsMessengerEXT &debugMessager, const VkApplicationInfo &appInfo,
            const std::vector<const char *> &layers);
}