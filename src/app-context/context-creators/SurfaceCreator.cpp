#include "SurfaceCreator.hpp"

#include "utils/Logger.hpp"

void ContextCreator::createSurface(VkInstance instance, VkSurfaceKHR &surface,
                                   GLFWwindow *window) {
  VkResult result =
      glfwCreateWindowSurface(instance, window, nullptr, &surface);
  Logger::checkStep("glfwCreateWindowSurface", result);
}