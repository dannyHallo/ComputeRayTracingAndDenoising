#pragma once

#include "Window.h"

class HoverWindow : public Window {
public:
  HoverWindow(int windowWidth, int windowHeight) : Window(WindowStyle::HOVER, windowWidth, windowHeight) {}
};