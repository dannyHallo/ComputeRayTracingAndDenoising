#pragma once

#include <deque>
#include <vector>

class VulkanApplicationContext;
class ColorPalette;

class FpsGui {
public:
  FpsGui(ColorPalette *colorPalette);
  void update(VulkanApplicationContext *appContext, double filteredFps);

private:
  ColorPalette *_colorPalette;
  std::deque<float> _fpsHistory;

  std::vector<float> _x{};
  std::vector<float> _y{};

  void _updateFpsHistData(double fps);
};