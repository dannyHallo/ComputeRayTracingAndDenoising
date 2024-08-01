#include "ApplicationInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void ApplicationInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  framesInFlight     = tomlConfigReader->getConfig<uint32_t>("Application.framesInFlight");
  isFramerateLimited = tomlConfigReader->getConfig<bool>("Application.isFramerateLimited");
}
