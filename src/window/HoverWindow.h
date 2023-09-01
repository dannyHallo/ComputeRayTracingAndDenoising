#pragma once

#include "Window.h"

static constexpr int kDefaultWindowWidth  = 400;
static constexpr int kDefaultWindowHeight = 300;
class HoverWindow : public Window {
public:
  HoverWindow(int windowWidth = kDefaultWindowWidth, int windowHeight = kDefaultWindowHeight)
      : Window(WindowStyle::HOVER, windowWidth, windowHeight) {}
};