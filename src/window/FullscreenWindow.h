#pragma once

#include "Window.h"

class FullscreenWindow : public Window {
public:
  FullscreenWindow() : Window(WindowStyle::FULLSCREEN) {}
};