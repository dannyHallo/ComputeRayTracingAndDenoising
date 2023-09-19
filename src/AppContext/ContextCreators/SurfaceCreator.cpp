#include "SurfaceCreator.h"

#include "utils/Logger.h"

void SurfaceCreator::create(VkInstance instance, VkSurfaceKHR &surface, GLFWwindow *window) {
  VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  Logger::checkStep("glfwCreateWindowSurface", result);
}