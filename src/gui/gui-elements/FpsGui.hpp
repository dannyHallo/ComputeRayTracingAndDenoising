#pragma once

#include <deque>
#include <vector>

class ColorPalette;
class FpsGui {
public:
  FpsGui(ColorPalette *colorPalette);
  void update(double filteredFps);
  void setActive(bool active);

private:
  bool _isActive = true;

  ColorPalette *_colorPalette;
  std::deque<float> _fpsHistory;

  std::vector<float> _x{};
  std::vector<float> _y{};

  void _updateFpsHistData(double fps);
};