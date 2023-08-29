#pragma once

#include "Window.h"

class MaximizedWindow : public Window {
public:
  MaximizedWindow() : Window(WindowStyle::MAXIMAZED) {}
};