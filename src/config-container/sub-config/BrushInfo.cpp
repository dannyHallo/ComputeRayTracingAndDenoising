#include "BrushInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void BrushInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  size     = tomlConfigReader->getConfig<float>("Brush.size");
  strength = tomlConfigReader->getConfig<float>("Brush.strength");
}
