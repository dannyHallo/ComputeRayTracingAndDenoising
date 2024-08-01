#include "BrushData.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

BrushData::BrushData(TomlConfigReader *tomlConfigReader) : _tomlConfigReader(tomlConfigReader) {
  _loadConfig();
}

void BrushData::_loadConfig() { size = _tomlConfigReader->getConfig<float>("Brush.size"); }
