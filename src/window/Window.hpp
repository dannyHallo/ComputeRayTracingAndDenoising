#pragma once

#include "GLFW/glfw3.h"

#ifdef APIENTRY
#undef APIENTRY
#endif

#include "volk.h"

#include "CursorInfo.hpp"
#include "KeyboardInfo.hpp"

#include <functional>
#include <vector>

enum class WindowStyle { kNone, kFullScreen, kMaximized, kHover };

class Logger;
class Window {
public:
  Window(WindowStyle windowStyle, Logger *logger, int widthIfWindowed = 400,
         int heightIfWindowed = 300);
  ~Window();

  // disable move and copy
  Window(const Window &)            = delete;
  Window &operator=(const Window &) = delete;
  Window(Window &&)                 = delete;
  Window &operator=(Window &&)      = delete;

  [[nodiscard]] GLFWwindow *getGlWindow() const { return _window; }
  [[nodiscard]] GLFWmonitor *getMonitor() const { return _monitor; }

  [[nodiscard]] WindowStyle getWindowStyle() const { return _windowStyle; }
  [[nodiscard]] CursorState getCursorState() const { return _cursorInfo.cursorState; }

  // be careful to use these two functions, you might want to query the
  // framebuffer size, not the window size
  [[nodiscard]] int getWindowWidth() const {
    int width  = 0;
    int height = 0;
    glfwGetWindowSize(_window, &width, &height);
    return width;
  }

  [[nodiscard]] int getWindowHeight() const {
    int width  = 0;
    int height = 0;
    glfwGetWindowSize(_window, &width, &height);
    return height;
  }

  [[nodiscard]] int getFrameBufferWidth() const {
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    return width;
  }

  [[nodiscard]] int getFrameBufferHeight() const {
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    return height;
  }

  [[nodiscard]] int getCursorXPos() const {
    double xPos = 0.F;
    double yPos = 0.F;
    glfwGetCursorPos(_window, &xPos, &yPos);
    return static_cast<int>(xPos);
  }

  [[nodiscard]] int getCursorYPos() const {
    double xPos = 0.F;
    double yPos = 0.F;
    glfwGetCursorPos(_window, &xPos, &yPos);
    return static_cast<int>(yPos);
  }

  void toggleWindowStyle();

  void setWindowStyle(WindowStyle newStyle);

  void showCursor();
  void hideCursor();
  void toggleCursor();

  void addCursorMoveCallback(std::function<void(CursorMoveInfo const &)> callback);
  void addCursorButtonCallback(std::function<void(CursorInfo const &)> callback);
  void addKeyboardCallback(std::function<void(KeyboardInfo const &)> callback);

  CursorInfo getCursorInfo() const { return _cursorInfo; }
  KeyboardInfo getKeyboardInfo() const { return _keyboardInfo; }

private:
  WindowStyle _windowStyle = WindowStyle::kNone;

  Logger *_logger;

  int _widthIfWindowed;
  int _heightIfWindowed;

  GLFWwindow *_window   = nullptr;
  GLFWmonitor *_monitor = nullptr;

  CursorInfo _cursorInfo;
  KeyboardInfo _keyboardInfo;

  // these are used to restore maximized window to its original size and pos
  int _titleBarHeight                  = 0;
  int _maximizedFullscreenClientWidth  = 0;
  int _maximizedFullscreenClientHeight = 0;

  std::vector<std::function<void(CursorMoveInfo)>> _cursorMoveCallbacks;
  std::vector<std::function<void(CursorInfo)>> _cursorButtonCallbacks;
  std::vector<std::function<void(KeyboardInfo)>> _keyboardCallbacks;

  void _resetCursorDelta();

  void _windowStyleToggleCallback(KeyboardInfo const &keyboardInfo);

  // these functions are restricted to be static functions
  static void _keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
  static void _cursorPosCallback(GLFWwindow *window, double xPos, double yPos);
  static void _mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
  static void _frameBufferResizeCallback(GLFWwindow *window, int width, int height);
};