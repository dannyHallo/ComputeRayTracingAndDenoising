#pragma once

class TomlConfigReader;

struct BrushInfo {
  float size{};

  void loadConfig(TomlConfigReader *tomlCOonfigReader);
};
