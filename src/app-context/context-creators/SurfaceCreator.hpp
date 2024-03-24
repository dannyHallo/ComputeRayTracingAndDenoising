#pragma once

#include "volk.h"

// glfw3 will define APIENTRY if it is not defined yet
#include "GLFW/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
// we undefine this to solve conflict with systemLog

class Logger;
namespace ContextCreator {
void createSurface(Logger *logger, VkInstance instance, VkSurfaceKHR &surface, GLFWwindow *window);
} // namespace ContextCreator