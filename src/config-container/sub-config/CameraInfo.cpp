#include "CameraInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void CameraInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  initHeight         = tomlConfigReader->getConfig<float>("Camera.initHeight");
  initYaw            = tomlConfigReader->getConfig<float>("Camera.initYaw");
  initPitch          = tomlConfigReader->getConfig<float>("Camera.initPitch");
  vFov               = tomlConfigReader->getConfig<float>("Camera.vFov");
  movementSpeed      = tomlConfigReader->getConfig<float>("Camera.movementSpeed");
  movementSpeedBoost = tomlConfigReader->getConfig<float>("Camera.movementSpeedBoost");
  mouseSensitivity   = tomlConfigReader->getConfig<float>("Camera.mouseSensitivity");
}
