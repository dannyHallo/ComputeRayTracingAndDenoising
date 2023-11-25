#include "SurfaceCreator.hpp"

#include "utils/Logger.hpp"

#include <cassert>

void ContextCreator::createSurface(Logger *logger, VkInstance instance, VkSurfaceKHR &surface,
                                   GLFWwindow *window) {
  VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  assert(result == VK_SUCCESS && "failed to create window surface");
}