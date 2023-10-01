#include "Window.h"
#include "utils/Logger.h"

namespace {
void frameBufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto *thisWindowClass =
      reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  thisWindowClass->setWindowSizeChanged(true);

  // Logger::print("Frame Width is changing to",
  //               thisWindowClass->getFrameBufferWidth());
  // Logger::print("Frame Height is changing to",
  //               thisWindowClass->getFrameBufferHeight());
}
} // namespace

Window::Window(WindowStyle windowStyle, int widthIfWindowed,
               int heightIfWindowed)
    : mWidthIfWindowed(widthIfWindowed), mHeightIfWindowed(heightIfWindowed) {
  auto result = glfwInit();
  assert(result == GLFW_TRUE && "Failed to initialize GLFW");

  mMonitor = glfwGetPrimaryMonitor();
  assert(mMonitor != nullptr && "Failed to get primary monitor");

  // get primary monitor for future maximize function
  // may be used to change mode for this program
  const GLFWvidmode *mode = glfwGetVideoMode(mMonitor);
  assert(mode != nullptr && "Failed to get video mode");

  // only OpenGL Api is supported, so no API here
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits); // adapt colors (notneeded)
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate); // adapt framerate

  // create a windowed fullscreen window temporalily, to obtain its property
  mWindow = glfwCreateWindow(mode->width, mode->height, "Loading window...",
                             nullptr, nullptr);
  glfwMaximizeWindow(mWindow);
  glfwGetWindowPos(mWindow, 0, &mTitleBarHeight);
  glfwGetFramebufferSize(mWindow, &mMaximizedFullscreenClientWidth,
                         &mMaximizedFullscreenClientHeight);

  // change the created window to the desired style
  setWindowStyle(windowStyle);

  mWindowStyle = windowStyle;

  if (mCursorState == CursorState::kInvisible) {
    hideCursor();
  } else {
    showCursor();
  }

  glfwSetWindowUserPointer(mWindow, this);
  glfwSetKeyCallback(mWindow, keyCallback);
  glfwSetFramebufferSizeCallback(mWindow, frameBufferResizeCallback);

  // Logger::print("Window created");
  // Logger::print("Frame Width is:", getFrameBufferWidth());
  // Logger::print("Frame Height is:", getFrameBufferHeight());
  // Logger::print("Window Width is:", getWindowWidth());
  // Logger::print("Window Height is:", getWindowHeight());
}

void Window::toggleWindowStyle() {
  switch (mWindowStyle) {
  case WindowStyle::kNone:
    assert(false && "Cannot toggle window style while it is none");
    break;
  case WindowStyle::kFullScreen:
    setWindowStyle(WindowStyle::kMaximized);
    break;
  case WindowStyle::kMaximized:
    setWindowStyle(WindowStyle::kHover);
    break;
  case WindowStyle::kHover:
    setWindowStyle(WindowStyle::kFullScreen);
    break;
  }
}

void Window::setWindowStyle(WindowStyle newStyle) {
  if (newStyle == mWindowStyle) {
    return;
  }

  const GLFWvidmode *mode = glfwGetVideoMode(mMonitor);
  assert(mode != nullptr && "Failed to get video mode");

  switch (newStyle) {
  case WindowStyle::kNone:
    assert(false && "Cannot set window style to none");
    break;

  case WindowStyle::kFullScreen:
    glfwSetWindowMonitor(mWindow, mMonitor, 0, 0, mode->width, mode->height,
                         mode->refreshRate);
    break;

  case WindowStyle::kMaximized:
    glfwSetWindowMonitor(mWindow, nullptr, 0, mTitleBarHeight,
                         mMaximizedFullscreenClientWidth,
                         mMaximizedFullscreenClientHeight, mode->refreshRate);
    break;

  case WindowStyle::kHover:
    int hoverWindowX = static_cast<int>(mMaximizedFullscreenClientWidth / 2.F -
                                        mWidthIfWindowed / 2.F);
    int hoverWindowY = static_cast<int>(mMaximizedFullscreenClientHeight / 2.F -
                                        mHeightIfWindowed / 2.F);
    glfwSetWindowMonitor(mWindow, nullptr, hoverWindowX, hoverWindowY,
                         mWidthIfWindowed, mHeightIfWindowed,
                         mode->refreshRate);
    break;
  }

  mWindowStyle = newStyle;
}

void Window::showCursor() {
  glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  mCursorState = CursorState::kVisible;
  glfwSetCursorPos(mWindow, getFrameBufferWidth() / 2.F,
                   getFrameBufferHeight() / 2.F);
}

void Window::hideCursor() {
  glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (glfwRawMouseMotionSupported() != 0) {
    glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }

  mCursorState = CursorState::kInvisible;
}

void Window::toggleCursor() {
  if (mCursorState == CursorState::kInvisible) {
    showCursor();
  } else {
    hideCursor();
  }
}

void Window::keyCallback(GLFWwindow *window, int key, int /*scancode*/,
                         int action, int /*mods*/) {
  auto *thisWindowClass =
      reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

  if (action == GLFW_PRESS || action == GLFW_RELEASE) {
    thisWindowClass->mKeyInputMap[key] = action == GLFW_PRESS;
  }
}