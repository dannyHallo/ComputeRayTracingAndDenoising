#include "SurfaceCreator.hpp"

#include "utils/logger/Logger.hpp"

void ContextCreator::createSurface(Logger * /*logger*/, VkInstance instance, VkSurfaceKHR &surface,
                                   GLFWwindow *window) {
  glfwCreateWindowSurface(instance, window, nullptr, &surface);
}