#include "SurfaceCreator.h"

#include "utils/logger.h"

void SurfaceCreator::create(VkInstance instance, VkSurfaceKHR &surface, GLFWwindow *window) {
  VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  logger::checkStep("glfwCreateWindowSurface", result);
}