#include "Window.h"
#include "utils/systemLog.h"

// static class variable functions like extern variables, they have external linkage and static storage duration
// we define this window pointer here as a handle in glfw callback function, it will be initialized after the first construction
// of this class
Window *Window::thisWindowClass;

Window::Window() : mCursorState(CURSOR_STATE_INVISIBLE) {
  thisWindowClass = this;

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

  if (mCursorState == CURSOR_STATE_INVISIBLE)
    hideCursor();
  else
    showCursor();

  glfwSetKeyCallback(mWindow, keyCallback);
}

Window::~Window() {
  print("Destroying object with type: window");
  glfwDestroyWindow(mWindow);
}

void Window::showCursor() {
  glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  mCursorState = CURSOR_STATE_VISIBLE;
}

void Window::hideCursor() {
  glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  if (glfwRawMouseMotionSupported()) {
    glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }
  mCursorState = CURSOR_STATE_INVISIBLE;
}

void Window::toggleCursor() {
  if (mCursorState == CURSOR_STATE_INVISIBLE) {
    showCursor();
  } else {
    hideCursor();
  }
}

void Window::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    // Direction keycodes
    switch (key) {
    case GLFW_KEY_W:
      thisWindowClass->mKeyInputBits |= W_BIT;
      break;
    case GLFW_KEY_A:
      thisWindowClass->mKeyInputBits |= A_BIT;
      break;
    case GLFW_KEY_S:
      thisWindowClass->mKeyInputBits |= S_BIT;
      break;
    case GLFW_KEY_D:
      thisWindowClass->mKeyInputBits |= D_BIT;
      break;
    case GLFW_KEY_SPACE:
      thisWindowClass->mKeyInputBits |= SPACE_BIT;
      break;
    case GLFW_KEY_LEFT_SHIFT:
      thisWindowClass->mKeyInputBits |= SHIFT_BIT;
      break;
    case GLFW_KEY_LEFT_CONTROL:
      thisWindowClass->mKeyInputBits |= CTRL_BIT;
      break;
    case GLFW_KEY_ESCAPE:
      thisWindowClass->mKeyInputBits |= ESC_BIT;
      break;
    case GLFW_KEY_TAB:
      thisWindowClass->mKeyInputBits |= TAB_BIT;
      break;
    }

    return;
  }

  if (action == GLFW_RELEASE) {
    switch (key) {
    case GLFW_KEY_W:
      thisWindowClass->mKeyInputBits &= ~W_BIT;
      break;
    case GLFW_KEY_A:
      thisWindowClass->mKeyInputBits &= ~A_BIT;
      break;
    case GLFW_KEY_S:
      thisWindowClass->mKeyInputBits &= ~S_BIT;
      break;
    case GLFW_KEY_D:
      thisWindowClass->mKeyInputBits &= ~D_BIT;
      break;
    case GLFW_KEY_SPACE:
      thisWindowClass->mKeyInputBits &= ~SPACE_BIT;
      break;
    case GLFW_KEY_LEFT_SHIFT:
      thisWindowClass->mKeyInputBits &= ~SHIFT_BIT;
      break;
    case GLFW_KEY_LEFT_CONTROL:
      thisWindowClass->mKeyInputBits &= ~CTRL_BIT;
      break;
    case GLFW_KEY_ESCAPE:
      thisWindowClass->mKeyInputBits &= ~ESC_BIT;
      break;
    case GLFW_KEY_TAB:
      thisWindowClass->mKeyInputBits &= ~TAB_BIT;
      break;
    }

    return;
  }
}