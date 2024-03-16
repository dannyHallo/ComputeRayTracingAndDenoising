#pragma once

#include "utils/incl/TomlIncl.hpp"
#include "utils/logger/Logger.hpp"

#include <memory>
#include <optional>
#include <string>

class TomlConfigReader {
public:
  TomlConfigReader(Logger *logger);

  template <class T> T getConfig(std::string const &configItemPath) {
    // first tries to get the config from CustomConfig.toml
    auto customConfigOpt = _tryGetConfigFromCustomConfig<T>(configItemPath);
    if (customConfigOpt.has_value()) {
      return customConfigOpt.value();
    }

    // fallback to DefaultConfig.toml
    auto defaultConfigOpt = _tryGetConfigFromDefaultConfig<T>(configItemPath);
    if (!defaultConfigOpt.has_value()) {
      _logger->error("TomlConfigReader::getConfig() failed to get default config at {}",
                     configItemPath);
    }
    return defaultConfigOpt.value();
  }

private:
  Logger *_logger;
  std::unique_ptr<toml::v3::parse_result> _defaultConfig;
  std::unique_ptr<toml::v3::parse_result> _customConfig;

  void _parseConfigsFromFile();

  template <class T>
  std::optional<T> _tryGetConfigFromDefaultConfig(std::string const &configItemPath) {
    return _defaultConfig->at_path(configItemPath).value<T>();
  }

  template <class T>
  std::optional<T> _tryGetConfigFromCustomConfig(std::string const &configItemPath) {
    if (!_customConfig->succeeded()) {
      return std::nullopt;
    }
    return _customConfig->at_path(configItemPath).value<T>();
  }
};