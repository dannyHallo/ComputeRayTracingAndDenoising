#pragma once

#include "Window.h"

static constexpr int kDefaultWindowWidth  = 1920;
static constexpr int kDefaultWindowHeight = 1080;
class HoverWindow : public Window {
public:
  HoverWindow(int windowWidth = kDefaultWindowWidth, int windowHeight = kDefaultWindowHeight)
      : Window(WindowStyle::HOVER, windowWidth, windowHeight) {}
};