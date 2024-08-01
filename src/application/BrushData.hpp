#pragma once

class TomlConfigReader;

struct BrushData {
  BrushData(TomlConfigReader *tomlConfigReader);

  float size{};

private:
  TomlConfigReader *_tomlConfigReader;

  void _loadConfig();
};
