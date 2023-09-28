#include "Window.h"
#include "utils/Logger.h"

namespace {
void frameBufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto *thisWindowClass =
      reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  thisWindowClass->setWindowSizeChanged(true);

  // debug functions
  Logger::print("window size changed");
  Logger::print("Frame Width is changing to:",
                thisWindowClass->getFrameBufferWidth());
  Logger::print("Frame Height is changing to:",
                thisWindowClass->getFrameBufferHeight());
  Logger::print("Window Width is changing to:",
                thisWindowClass->getWindowWidth());
  Logger::print("Window Height is changing to:",
                thisWindowClass->getWindowHeight());
}
} // namespace

Window::Window(WindowStyle windowStyle, int widthIfWindowed,
               int heightIfWindowed)
    : mWindowStyle(windowStyle), mCursorState(CursorState::INVISIBLE),
      mWidthIfWindowed(widthIfWindowed), mHeightIfWindowed(heightIfWindowed),
      mKeyInputBits(0) {
  auto result = glfwInit();
  assert(result == GLFW_TRUE && "Failed to initialize GLFW");

  mMonitor = glfwGetPrimaryMonitor();
  assert(mMonitor != nullptr && "Failed to get primary monitor");

  // get primary monitor for future maximize function
  const GLFWvidmode *mode =
      glfwGetVideoMode(mMonitor); // may be used to change mode for this program
  assert(mode != nullptr && "Failed to get video mode");

  glfwWindowHint(GLFW_CLIENT_API,
                 GLFW_NO_API); // only OpenGL Api is supported, so no API here

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits); // adapt colors (notneeded)
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate); // adapt framerate

  // set window size
  switch (windowStyle) {
  case WindowStyle::FULLSCREEN:
    mWindow = glfwCreateWindow(mode->width, mode->height, "Loading window...",
                               mMonitor, nullptr);
    break;
  case WindowStyle::MAXIMAZED:
    mWindow = glfwCreateWindow(mode->width, mode->height, "Loading window...",
                               nullptr, nullptr);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    break;
  case WindowStyle::HOVER:
    mWindow = glfwCreateWindow(mWidthIfWindowed, mHeightIfWindowed,
                               "Loading window...", nullptr, nullptr);
    break;
  }

  if (mCursorState == CursorState::INVISIBLE) {
    hideCursor();
  } else {
    showCursor();
  }

  glfwSetWindowUserPointer(mWindow, this);
  glfwSetKeyCallback(mWindow, keyCallback);
  glfwSetFramebufferSizeCallback(mWindow, frameBufferResizeCallback);

  // debug functions
  Logger::print("Window created");
  Logger::print("Frame Width is:", getFrameBufferWidth());
  Logger::print("Frame Height is:", getFrameBufferHeight());
  Logger::print("Window Width is:", getWindowWidth());
  Logger::print("Window Height is:", getWindowHeight());
}

void Window::showCursor() {
  glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  mCursorState = CursorState::VISIBLE;
}

void Window::hideCursor() {
  glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (glfwRawMouseMotionSupported() != 0) {
    glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }

  mCursorState = CursorState::INVISIBLE;
}

void Window::toggleCursor() {
  if (mCursorState == CursorState::INVISIBLE) {
    showCursor();
  } else {
    hideCursor();
  }
}

void Window::keyCallback(GLFWwindow *window, int key, int /*scancode*/,
                         int action, int /*mods*/) {
  auto *thisWindowClass =
      reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

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
  } else if (action == GLFW_RELEASE) {
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