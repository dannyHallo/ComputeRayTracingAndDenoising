#include "Window.h"
#include "utils/systemLog.h"

Window::Window() {
  glfwInit();

  mMonitor                = glfwGetPrimaryMonitor();    // Get primary monitor for future maximize function
  const GLFWvidmode *mode = glfwGetVideoMode(mMonitor); // May be used to change mode for this program

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Only OpenGL Api is supported, so no API here

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);       // Adapt colors (not needed)
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate); // Adapt framerate

  // Set window size
  switch (windowSize) {
  case WINDOW_SIZE_FULLSCREEN:
    mWindow = glfwCreateWindow(mode->width, mode->height, "Loading...", mMonitor, nullptr);
    break;
  case WINDOW_SIZE_MAXIMAZED:
    mWindow = glfwCreateWindow(mode->width, mode->height, "Loading...", nullptr, nullptr);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    break;
  case WINDOW_SIZE_HOVER:
    mWindow = glfwCreateWindow(widthWindowed, heightWindowed, "Loading...", nullptr, nullptr);
    break;
  }

  hideCursor();
}

Window::~Window() {
  print("Destroying object with type: window");
  glfwDestroyWindow(mWindow);
}