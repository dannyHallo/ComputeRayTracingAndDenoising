#pragma once

class TomlConfigReader;

struct BrushInfo {
  float size{};
  float strength{};

  void loadConfig(TomlConfigReader *tomlCOonfigReader);
};
