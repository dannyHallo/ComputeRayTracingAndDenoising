#pragma once

#include "utils/incl/TomlIncl.hpp"

#include <memory>
#include <string>

class Logger;

class TomlConfigReader {
public:
  TomlConfigReader(Logger *logger);

  template <class T> T getConfig(std::string const &configItemPath);

private:
  Logger *_logger;
  std::unique_ptr<toml::v3::parse_result> _defaultConfig;
  std::unique_ptr<toml::v3::parse_result> _userConfig;

  void _parseConfigsFromFile();

  template <class T>
  std::optional<T> _tryGetConfigFromDefaultConfig(std::string const &configItemPath);

  template <class T>
  std::optional<T> _tryGetConfigFromUserConfig(std::string const &configItemPath);

  void _testing();
};