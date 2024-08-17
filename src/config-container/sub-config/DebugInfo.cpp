#include "DebugInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void DebugInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  debugB1         = tomlConfigReader->getConfig<bool>("DebugInfo.debugB1");
  debugF1         = tomlConfigReader->getConfig<float>("DebugInfo.debugF1");
  debugI1         = tomlConfigReader->getConfig<int>("DebugInfo.debugI1");
  auto const &dc1 = tomlConfigReader->getConfig<std::array<float, 3>>("DebugInfo.debugC1");
  debugC1         = glm::vec3(dc1.at(0), dc1.at(1), dc1.at(2));
}
