#include "ShadowMapCameraInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void ShadowMapCameraInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  range = tomlConfigReader->getConfig<float>("ShadowMapCamera.range");
}
