#pragma once

#include "utils/color-palette/Color.hpp"

class TomlConfigReader;

struct ImguiManagerInfo {
  float fontSize;
  Color fpsGuiColor;
  Color menuBarBackgroundColor;
  Color popupBackgroundColor;

  void loadConfig(TomlConfigReader *tomlConfigReader);
};