#pragma once

#include "GLFW/glfw3.h"
#include "utils/vulkan.h"

#define WINDOW_SIZE_FULLSCREEN 0
#define WINDOW_SIZE_MAXIMAZED 1
#define WINDOW_SIZE_HOVER 2

#define CURSOR_STATE_INVISIBLE 0
#define CURSOR_STATE_VISIBLE 1

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
public:
  Window();
  ~Window();

  GLFWwindow *getWindow() const { return mWindow; }
  GLFWmonitor *getMonitor() const { return mMonitor; }
  uint32_t getKeyInputs() const { return mKeyInputBits; }
  uint8_t getCursorState() const { return mCursorState; }

  void showCursor();
  void hideCursor();
  void toggleCursor();

  void disableInputBit(uint32_t bitToBeDisabled) { mKeyInputBits &= ~bitToBeDisabled; }

private:
  static Window *thisWindowClass;
  const uint8_t windowSize      = WINDOW_SIZE_MAXIMAZED;
  const uint32_t widthWindowed  = 1280;
  const uint32_t heightWindowed = 1280;

  uint8_t mCursorState;
  uint32_t mKeyInputBits = 0;

  GLFWwindow *mWindow;
  GLFWmonitor *mMonitor;

  static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
};