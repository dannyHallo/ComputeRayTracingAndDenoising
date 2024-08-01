#include "ConfigContainer.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

#include "sub-config/ApplicationInfo.hpp"
#include "sub-config/BrushInfo.hpp"
#include "sub-config/CameraInfo.hpp"
#include "sub-config/ImguiManagerInfo.hpp"
#include "sub-config/ShadowMapCameraInfo.hpp"
#include "sub-config/SvoBuilderInfo.hpp"
#include "sub-config/SvoTracerInfo.hpp"
#include "sub-config/SvoTracerTweakingInfo.hpp"

ConfigContainer::ConfigContainer(Logger *logger)
    : applicationInfo(std::make_unique<ApplicationInfo>()),
      brushInfo(std::make_unique<BrushInfo>()), cameraInfo(std::make_unique<CameraInfo>()),
      imguiManagerInfo(std::make_unique<ImguiManagerInfo>()),
      shadowMapCameraInfo(std::make_unique<ShadowMapCameraInfo>()),
      svoBuilderInfo(std::make_unique<SvoBuilderInfo>()),
      svoTracerInfo(std::make_unique<SvoTracerInfo>()),
      svoTracerTweakingInfo(std::make_unique<SvoTracerTweakingInfo>()), _logger(logger) {
  _loadConfig();
}

ConfigContainer::~ConfigContainer() = default;

void ConfigContainer::_loadConfig() {
  TomlConfigReader tomlConfigReader{_logger};

  applicationInfo->loadConfig(&tomlConfigReader);
  brushInfo->loadConfig(&tomlConfigReader);
  cameraInfo->loadConfig(&tomlConfigReader);
  imguiManagerInfo->loadConfig(&tomlConfigReader);
  shadowMapCameraInfo->loadConfig(&tomlConfigReader);
  svoBuilderInfo->loadConfig(&tomlConfigReader);
  svoTracerInfo->loadConfig(&tomlConfigReader);
  svoTracerTweakingInfo->loadConfig(&tomlConfigReader);
}
