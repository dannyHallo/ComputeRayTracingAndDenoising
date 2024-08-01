#pragma once

class TomlConfigReader;

struct ShadowMapCameraInfo {
  float range{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
