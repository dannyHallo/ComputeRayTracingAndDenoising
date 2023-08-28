#pragma once

#include "glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif

#include "utils/vulkan.h"

enum WindowSize { FULLSCREEN, MAXIMAZED, HOVER };
enum CursorState { INVISIBLE, VISIBLE };

#define W_BIT 1
#define A_BIT 2
#define S_BIT 4
#define D_BIT 8
#define SPACE_BIT 16
#define SHIFT_BIT 32
#define CTRL_BIT 64
#define ESC_BIT 128
#define TAB_BIT 256

class Window {
  static Window *thisWindowClass;

  int mWindowSize;
  uint32_t mWidthIfWindowed;
  uint32_t mHeightIfWindowed;
  uint8_t mCursorState;
  uint32_t mKeyInputBits;

  GLFWwindow *mWindow;
  GLFWmonitor *mMonitor;

public:
  Window(int windowSize = WindowSize::FULLSCREEN,
         uint32_t widthIfWindowed = 1280, uint32_t heightIfWindowed = 1280);
  ~Window() { glfwDestroyWindow(mWindow); }

  GLFWwindow *getWindow() const { return mWindow; }
  GLFWmonitor *getMonitor() const { return mMonitor; }
  uint32_t getKeyInputs() const { return mKeyInputBits; }
  uint8_t getCursorState() const { return mCursorState; }

  void showCursor();
  void hideCursor();
  void toggleCursor();

  void disableInputBit(uint32_t bitToBeDisabled) {
    mKeyInputBits &= ~bitToBeDisabled;
  }

private:
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                          int mods);
};