#include "ImguiManagerInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void ImguiManagerInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  fontSize = tomlConfigReader->getConfig<float>("ImguiManager.fontSize");
}
