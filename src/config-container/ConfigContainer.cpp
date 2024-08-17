#include "ConfigContainer.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

#include "sub-config/ApplicationInfo.hpp"
#include "sub-config/AtmosInfo.hpp"
#include "sub-config/BrushInfo.hpp"
#include "sub-config/CameraInfo.hpp"
#include "sub-config/DebugInfo.hpp"
#include "sub-config/ImguiManagerInfo.hpp"
#include "sub-config/PostProcessingInfo.hpp"
#include "sub-config/ShadowMapCameraInfo.hpp"
#include "sub-config/SvoTracerInfo.hpp"
#include "sub-config/TerrainInfo.hpp"
#include "sub-config/TracingInfo.hpp"

ConfigContainer::ConfigContainer(Logger *logger) : _logger(logger) {
  applicationInfo     = std::make_unique<ApplicationInfo>();
  atmosInfo           = std::make_unique<AtmosInfo>();
  brushInfo           = std::make_unique<BrushInfo>();
  cameraInfo          = std::make_unique<CameraInfo>();
  debugInfo           = std::make_unique<DebugInfo>();
  imguiManagerInfo    = std::make_unique<ImguiManagerInfo>();
  postProcessingInfo  = std::make_unique<PostProcessingInfo>();
  shadowMapCameraInfo = std::make_unique<ShadowMapCameraInfo>();
  svoTracerInfo       = std::make_unique<SvoTracerInfo>();
  terrainInfo         = std::make_unique<TerrainInfo>();
  tracingInfo         = std::make_unique<TracingInfo>();

  _loadConfig();
}

ConfigContainer::~ConfigContainer() = default;

void ConfigContainer::_loadConfig() {
  TomlConfigReader tomlConfigReader{_logger};

  applicationInfo->loadConfig(&tomlConfigReader);
  atmosInfo->loadConfig(&tomlConfigReader);
  brushInfo->loadConfig(&tomlConfigReader);
  cameraInfo->loadConfig(&tomlConfigReader);
  debugInfo->loadConfig(&tomlConfigReader);
  imguiManagerInfo->loadConfig(&tomlConfigReader);
  postProcessingInfo->loadConfig(&tomlConfigReader);
  shadowMapCameraInfo->loadConfig(&tomlConfigReader);
  svoTracerInfo->loadConfig(&tomlConfigReader);
  terrainInfo->loadConfig(&tomlConfigReader);
  tracingInfo->loadConfig(&tomlConfigReader);
}
