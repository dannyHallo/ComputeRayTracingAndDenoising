#pragma once

#include "GLFW/glfw3.h"
#include "utils/vulkan.h"

#define WINDOW_SIZE_FULLSCREEN 0
#define WINDOW_SIZE_MAXIMAZED 1
#define WINDOW_SIZE_HOVER 2

class Window {
public:
  Window();
  ~Window();
  GLFWwindow *getWindow() const { return mWindow; }
  GLFWmonitor *getMonitor() const { return mMonitor; }

  void showCursor() { glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
  void hideCursor() {
    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
  }

private:
  const uint8_t windowSize      = WINDOW_SIZE_MAXIMAZED;
  const uint32_t widthWindowed  = 1280;
  const uint32_t heightWindowed = 1280;

  GLFWwindow *mWindow;
  GLFWmonitor *mMonitor;
};