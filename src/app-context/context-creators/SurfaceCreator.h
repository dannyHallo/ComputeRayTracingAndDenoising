#pragma once

#include "utils/vulkan.h"

// glfw3 will define APIENTRY if it is not defined yet
#include "glfw/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
// we undefine this to solve conflict with systemLog

namespace SurfaceCreator {
void create(VkInstance instance, VkSurfaceKHR &surface, GLFWwindow *window);
} // namespace SurfaceCreator