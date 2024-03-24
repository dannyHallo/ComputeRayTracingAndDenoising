#pragma once

#include "volk.h"

#include <vector>

class Logger;
namespace ContextCreator {
void createInstance(Logger *logger, VkInstance &instance, VkDebugUtilsMessengerEXT &debugMessager,
                    const VkApplicationInfo &appInfo, const std::vector<const char *> &layers);
} // namespace ContextCreator