#define TOML_IMPLEMENTATION
#include "TomlConfigReader.hpp"

#include "utils/config/RootDir.h"

static std::string const kDefaultConfigName = "DefaultConfig.toml";
static std::string const kCustomConfigName  = "CustomConfig.toml";

namespace {
std::string _makeConfigPath(std::string const &configName) {
  return kPathToResourceFolder + "configs/" + configName;
}
}; // namespace

TomlConfigReader::TomlConfigReader(Logger *logger) : _logger(logger) { _parseConfigsFromFile(); }

void TomlConfigReader::_parseConfigsFromFile() {
  std::string defaultConfigPath = _makeConfigPath(kDefaultConfigName);
  std::string customConfigPath  = _makeConfigPath(kCustomConfigName);

  _defaultConfig = std::make_unique<toml::v3::parse_result>(toml::parse_file(defaultConfigPath));
  if (!_defaultConfig->succeeded()) {
    _logger->error("TomlConfigReader::TomlConfigReader() failed to parse default config at {}",
                   defaultConfigPath);
  }
  _customConfig = std::make_unique<toml::v3::parse_result>(toml::parse_file(customConfigPath));
}
