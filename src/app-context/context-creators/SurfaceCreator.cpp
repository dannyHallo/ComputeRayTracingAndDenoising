#include "SurfaceCreator.hpp"

#include "utils/Logger.hpp"

void ContextCreator::createSurface(Logger *logger, VkInstance instance, VkSurfaceKHR &surface,
                                   GLFWwindow *window) {
  VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  logger->checkStep("glfwCreateWindowSurface", result);
}