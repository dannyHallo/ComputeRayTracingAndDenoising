#include "Window.hpp"

#include <cassert>

Window::Window(WindowStyle windowStyle, int widthIfWindowed, int heightIfWindowed)
    : _widthIfWindowed(widthIfWindowed), _heightIfWindowed(heightIfWindowed) {
  glfwInit();

  _monitor = glfwGetPrimaryMonitor();
  assert(_monitor != nullptr && "failed to get primary monitor");

  // get primary monitor for future maximize function
  // may be used to change mode for this program
  const GLFWvidmode *mode = glfwGetVideoMode(_monitor);
  assert(mode != nullptr && "failed to get video mode");

  // only OpenGL Api is supported, so no API here
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);       // adapt colors (notneeded)
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate); // adapt framerate

  // create a windowed fullscreen window temporalily, to obtain its property
  _window = glfwCreateWindow(mode->width, mode->height, "Voxel Tracer v1.0", nullptr, nullptr);
  glfwMaximizeWindow(_window);
  glfwGetWindowPos(_window, nullptr, &_titleBarHeight);
  glfwGetFramebufferSize(_window, &_maximizedFullscreenClientWidth,
                         &_maximizedFullscreenClientHeight);

  // change the created window to the desired style
  setWindowStyle(windowStyle);

  _windowStyle = windowStyle;

  if (_cursorState == CursorState::kInvisible) {
    hideCursor();
  } else {
    showCursor();
  }

  glfwSetWindowUserPointer(_window, this); // set this pointer to the window class
  glfwSetKeyCallback(_window, _keyCallback);
  glfwSetCursorPosCallback(_window, _cursorPosCallback);
  glfwSetFramebufferSizeCallback(_window, _frameBufferResizeCallback);
}

Window::~Window() {
  glfwDestroyWindow(_window);
  glfwTerminate();
}

void Window::toggleWindowStyle() {
  switch (_windowStyle) {
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
  if (newStyle == _windowStyle) {
    return;
  }

  const GLFWvidmode *mode = glfwGetVideoMode(_monitor);
  assert(mode != nullptr && "Failed to get video mode");

  switch (newStyle) {
  case WindowStyle::kNone:
    assert(false && "Cannot set window style to none");
    break;

  case WindowStyle::kFullScreen:
    glfwSetWindowMonitor(_window, _monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    break;

  case WindowStyle::kMaximized:
    glfwSetWindowMonitor(_window, nullptr, 0, _titleBarHeight, _maximizedFullscreenClientWidth,
                         _maximizedFullscreenClientHeight, mode->refreshRate);
    break;

  case WindowStyle::kHover:
    int hoverWindowX = static_cast<int>(static_cast<float>(_maximizedFullscreenClientWidth) / 2.F -
                                        static_cast<float>(_widthIfWindowed) / 2.F);
    int hoverWindowY = static_cast<int>(static_cast<float>(_maximizedFullscreenClientHeight) / 2.F -
                                        static_cast<float>(_heightIfWindowed) / 2.F);
    glfwSetWindowMonitor(_window, nullptr, hoverWindowX, hoverWindowY, _widthIfWindowed,
                         _heightIfWindowed, mode->refreshRate);
    break;
  }

  _windowStyle = newStyle;
}

void Window::showCursor() {
  glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  _cursorState = CursorState::kVisible;
  glfwSetCursorPos(_window, static_cast<float>(getFrameBufferWidth()) / 2.F,
                   static_cast<float>(getFrameBufferHeight()) / 2.F);
}

void Window::hideCursor() {
  glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (glfwRawMouseMotionSupported() != 0) {
    glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }

  _cursorState = CursorState::kInvisible;
}

void Window::toggleCursor() {
  if (_cursorState == CursorState::kInvisible) {
    showCursor();
  } else {
    hideCursor();
  }
}

void Window::addMouseCallback(std::function<void(float, float)> callback) {
  _mouseCallbacks.emplace_back(std::move(callback));
}

void Window::_keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
  auto *thisWindowClass = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

  if (action == GLFW_PRESS || action == GLFW_RELEASE) {
    thisWindowClass->_keyInputMap[key] = action == GLFW_PRESS;
  }
}

void Window::_cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
  auto *thisWindowClass = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

  static float lastX;
  static float lastY;
  static bool firstMouse = true;

  if (firstMouse) {
    lastX      = static_cast<float>(xpos);
    lastY      = static_cast<float>(ypos);
    firstMouse = false;
  }

  thisWindowClass->_mouseDeltaX = static_cast<float>(xpos) - lastX;
  // inverted y axis
  thisWindowClass->_mouseDeltaY = lastY - static_cast<float>(ypos);

  lastX = static_cast<float>(xpos);
  lastY = static_cast<float>(ypos);

  if (thisWindowClass->_mouseCallbacks.empty()) {
    return;
  }

  for (auto &callback : thisWindowClass->_mouseCallbacks) {
    callback(thisWindowClass->_mouseDeltaX, thisWindowClass->_mouseDeltaY);
  }
}

void Window::_frameBufferResizeCallback(GLFWwindow *window, int /*width*/, int /*height*/) {
  auto *thisWindowClass = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  thisWindowClass->setWindowSizeChanged(true);
}
