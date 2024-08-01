#pragma once

class TomlConfigReader;

struct ApplicationInfo {
  int framesInFlight{};
  bool isFramerateLimited{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
