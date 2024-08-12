#pragma once

#include <deque>
#include <vector>

struct ConfigContainer;

class VulkanApplicationContext;
class Logger;
class Window;

class FpsGui {
public:
  FpsGui(Logger *logger, ConfigContainer *configContainer, Window *window);
  void update(VulkanApplicationContext *appContext, double filteredFps);

private:
  Logger *_logger;
  ConfigContainer *_configContainer;

  Window *_window;

  std::deque<float> _fpsHistory;

  std::vector<float> _x{};
  std::vector<float> _y{};

  void _updateFpsHistData(double fps);
};