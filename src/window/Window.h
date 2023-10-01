#pragma once

#include "GLFW/glfw3.h"
#include <cstdint>

#ifdef APIENTRY
#undef APIENTRY
#endif

#include "utils/Vulkan.h"

#include <map>

enum class WindowStyle { kNone, kFullScreen, kMaximized, kHover };
enum class CursorState { kNone, kInvisible, kVisible };

class Window {
  WindowStyle mWindowStyle = WindowStyle::kNone;
  CursorState mCursorState = CursorState::kInvisible;

  int mWidthIfWindowed;
  int mHeightIfWindowed;
  std::map<int, bool> mKeyInputMap;

  bool mWindowSizeChanged = false;

  GLFWwindow *mWindow   = nullptr;
  GLFWmonitor *mMonitor = nullptr;

  // these are used to restore maximized window to its original size and pos
  int mTitleBarHeight                  = 0;
  int mMaximizedFullscreenClientWidth  = 0;
  int mMaximizedFullscreenClientHeight = 0;

public:
  // universal constructor, the last two parameters only will be used to
  // construct a hover window
  Window(WindowStyle windowStyle, int widthIfWindowed = 400,
         int heightIfWindowed = 300);

  ~Window() { glfwDestroyWindow(mWindow); }

  // disable move and copy
  Window(const Window &)            = delete;
  Window &operator=(const Window &) = delete;
  Window(Window &&)                 = delete;
  Window &operator=(Window &&)      = delete;

  [[nodiscard]] GLFWwindow *getGlWindow() const { return mWindow; }
  [[nodiscard]] GLFWmonitor *getMonitor() const { return mMonitor; }
  [[nodiscard]] bool isInputBitActive(int inputBit) {
    return mKeyInputMap.contains(inputBit) && mKeyInputMap[inputBit];
  }

  [[nodiscard]] WindowStyle getWindowStyle() const { return mWindowStyle; }
  [[nodiscard]] CursorState getCursorState() const { return mCursorState; }
  [[nodiscard]] bool windowSizeChanged() const { return mWindowSizeChanged; }

  // be careful to use these two functions, you might want to query the
  // framebuffer size, not the window size
  [[nodiscard]] int getWindowWidth() const {
    int width, height;
    glfwGetWindowSize(mWindow, &width, &height);
    return width;
  }
  [[nodiscard]] int getWindowHeight() const {
    int width, height;
    glfwGetWindowSize(mWindow, &width, &height);
    return height;
  }
  [[nodiscard]] int getFrameBufferWidth() const {
    int width, height;
    glfwGetFramebufferSize(mWindow, &width, &height);
    return width;
  }
  [[nodiscard]] int getFrameBufferHeight() const {
    int width, height;
    glfwGetFramebufferSize(mWindow, &width, &height);
    return height;
  }

  void toggleWindowStyle();

  void setWindowStyle(WindowStyle newStyle);

  void setWindowSizeChanged(bool windowSizeChanged) {
    mWindowSizeChanged = windowSizeChanged;
  }

  void showCursor();
  void hideCursor();
  void toggleCursor();

  void disableInputBit(int bitToBeDisabled) {
    mKeyInputMap[bitToBeDisabled] = false;
  }

private:
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                          int mods);
};