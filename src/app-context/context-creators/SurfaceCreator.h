#pragma once

#include "utils/Vulkan.h"

// glfw3 will define APIENTRY if it is not defined yet
#include "GLFW/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
// we undefine this to solve conflict with systemLog

namespace SurfaceCreator {
void create(VkInstance instance, VkSurfaceKHR &surface, GLFWwindow *window);
} // namespace SurfaceCreator