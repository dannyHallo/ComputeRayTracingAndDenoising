#include "Window.hpp"

#include "application/BlockState.hpp"
#include "utils/event-dispatcher/GlobalEventDispatcher.hpp"
#include "utils/event-types/EventType.hpp"

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
  glfwSetMouseButtonCallback(_window, _mouseButtonCallback);
  glfwSetFramebufferSizeCallback(_window, _frameBufferResizeCallback);

  addKeyboardCallback(
      [this](KeyboardInfo const &keyboardInfo) { _windowStyleToggleCallback(keyboardInfo); });
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
  _resetCursorDelta();
}

void Window::addCursorMoveCallback(std::function<void(CursorMoveInfo const &)> callback) {
  _cursorMoveCallbacks.emplace_back(std::move(callback));
}

void Window::addCursorButtonCallback(std::function<void(CursorInfo const &)> callback) {
  _cursorButtonCallbacks.emplace_back(std::move(callback));
}

void Window::addKeyboardCallback(std::function<void(KeyboardInfo const &)> callback) {
  _keyboardCallbacks.emplace_back(std::move(callback));
}

void Window::_resetCursorDelta() { _cursorInfo.cursorMoveInfo.firstMove = true; }

void Window::_keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
  auto *thisWindowClass = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

  KeyboardInfo &ki = thisWindowClass->_keyboardInfo;
  // update ki first
  if (action == GLFW_PRESS || action == GLFW_RELEASE) {
    ki.activeKeyMap[key] = (action == GLFW_PRESS);
  }

  for (auto &callback : thisWindowClass->_keyboardCallbacks) {
    callback(ki);
  }

  // consume keys
  ki.disableInputBit(GLFW_KEY_E);
  ki.disableInputBit(GLFW_KEY_F);
}

void Window::_cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
  auto *thisWindow = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

  CursorMoveInfo &cmi = thisWindow->_cursorInfo.cursorMoveInfo;
  cmi.currentX        = xpos;
  cmi.currentY        = ypos;

  if (cmi.firstMove) {
    cmi.lastX     = xpos;
    cmi.lastY     = ypos;
    cmi.firstMove = false;
  }

  cmi.dx = xpos - cmi.lastX;
  cmi.dy = ypos - cmi.lastY;
  // invert y axis
  cmi.dy *= -1.F;

  cmi.lastX = xpos;
  cmi.lastY = ypos;

  for (auto &callback : thisWindow->_cursorMoveCallbacks) {
    callback(cmi);
  }
}

void Window::_mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
  auto *thisWindow = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

  // update the cursor button related info
  CursorInfo &ci         = thisWindow->_cursorInfo;
  ci.leftButtonPressed   = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  ci.rightButtonPressed  = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  ci.middleButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

  for (auto &callback : thisWindow->_cursorButtonCallbacks) {
    callback(ci);
  }
}

void Window::_windowStyleToggleCallback(KeyboardInfo const &keyboardInfo) {
  if (keyboardInfo.isKeyPressed(GLFW_KEY_F)) {
    auto *thisWindow = reinterpret_cast<Window *>(glfwGetWindowUserPointer(_window));
    thisWindow->toggleWindowStyle();

    uint32_t blockStateBits = BlockState::kWindowResized;
    GlobalEventDispatcher::get().trigger<E_RenderLoopBlockRequest>(
        E_RenderLoopBlockRequest{blockStateBits});
  }
}

void Window::_frameBufferResizeCallback(GLFWwindow *window, int /*width*/, int /*height*/) {
  uint32_t blockStateBits = BlockState::kWindowResized;
  GlobalEventDispatcher::get().trigger<E_RenderLoopBlockRequest>(
      E_RenderLoopBlockRequest{blockStateBits});
}
