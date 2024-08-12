#pragma once

#include <deque>
#include <vector>

struct ConfigContainer;

class VulkanApplicationContext;

class FpsGui {
public:
  FpsGui(ConfigContainer *configContainer);
  void update(VulkanApplicationContext *appContext, double filteredFps);

private:
  ConfigContainer *_configContainer;

  std::deque<float> _fpsHistory;

  std::vector<float> _x{};
  std::vector<float> _y{};

  void _updateFpsHistData(double fps);
};