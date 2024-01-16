#pragma once

#include <deque>
#include <vector>

class FpsGui {
public:
  FpsGui();
  void update(double filteredFps);
  void setActive(bool active);

private:
  bool _isActive = true;
  std::deque<float> _fpsHistory;

  std::vector<float> _x{};
  std::vector<float> _y{};

  void _updateFpsHistData(double fps);
};