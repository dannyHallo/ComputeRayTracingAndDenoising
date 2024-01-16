#pragma once

#include <deque>

class FpsGui {
public:
  FpsGui();
  void update(float fps);
  void setActive(bool active);

private:
  bool _isActive = true;
  std::deque<float> _fpsHistory;

  void _updateFpsHistData(float fps);
};