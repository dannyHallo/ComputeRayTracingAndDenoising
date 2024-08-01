#pragma once

class TomlConfigReader;

struct ImguiManagerInfo {
  float fontSize;

  void loadConfig(TomlConfigReader *tomlConfigReader);
};