#include "SurfaceCreator.hpp"

#include "utils/logger/Logger.hpp"

void ContextCreator::createSurface(Logger *logger, VkInstance instance, VkSurfaceKHR &surface,
                                   GLFWwindow *window) {
  VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  if (res != VK_SUCCESS) {
    logger->error("failed to create window surface!");
  }
}
