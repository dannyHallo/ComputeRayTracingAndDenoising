#include "TomlConfigReader.hpp"

#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <optional>

static std::string const kDefaultConfigName = "DefaultConfig.toml";
static std::string const kUserConfigName    = "UserConfig.toml";

namespace {
std::string _makeConfigPath(std::string const &configName) {
  return kPathToResourceFolder + "configs/" + configName;
}
}; // namespace

TomlConfigReader::TomlConfigReader(Logger *logger) : _logger(logger) {
  _parseConfigsFromFile();
  _testing();
}

void TomlConfigReader::_parseConfigsFromFile() {
  std::string defaultConfigPath = _makeConfigPath(kDefaultConfigName);
  std::string userConfigPath    = _makeConfigPath(kUserConfigName);

  _defaultConfig = std::make_unique<toml::v3::parse_result>(toml::parse_file(defaultConfigPath));
  if (!_defaultConfig->succeeded()) {
    _logger->error("TomlConfigReader::TomlConfigReader() failed to parse default config at {}",
                   defaultConfigPath);
  }
  _userConfig = std::make_unique<toml::v3::parse_result>(toml::parse_file(userConfigPath));
}

template <class T>
std::optional<T>
TomlConfigReader::_tryGetConfigFromDefaultConfig(std::string const &configItemPath) {
  return _defaultConfig->at_path(configItemPath).value<T>();
}

template <class T>
std::optional<T> TomlConfigReader::_tryGetConfigFromUserConfig(std::string const &configItemPath) {
  if (!_userConfig->succeeded()) {
    return std::nullopt;
  }
  return _userConfig->at_path(configItemPath).value<T>();
}

template <class T> T TomlConfigReader::getConfig(std::string const &configItemPath) {
  auto userConfigOpt    = _tryGetConfigFromUserConfig<T>(configItemPath);
  auto defaultConfigOpt = _tryGetConfigFromDefaultConfig<T>(configItemPath);

  if (!defaultConfigOpt.has_value()) {
    _logger->error("TomlConfigReader::getConfig() failed to get default config at {}",
                   configItemPath);
  }

  return userConfigOpt.has_value() ? userConfigOpt.value() : defaultConfigOpt.value();
}

void TomlConfigReader::_testing() {
  _logger->info("val: {}", getConfig<int>("Application.frames_in_flight"));
}