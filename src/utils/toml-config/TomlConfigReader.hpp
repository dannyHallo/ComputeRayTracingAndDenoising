#pragma once

#include "utils/logger/Logger.hpp"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.hpp" // IWYU pragma: export

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

template <class T> struct ArrayTrait : std::false_type {
  using type = void;
};
template <class T, std::size_t N> struct ArrayTrait<std::array<T, N>> : std::true_type {
  using type                        = T;
  static constexpr std::size_t size = N;
};

class TomlConfigReader {
private:
  enum class ConfigType { kDefault, kCustom };

public:
  TomlConfigReader(Logger *logger);

  // T can be any type that toml++ supports, or an array containing any type that toml++ supports
  template <class T> T getConfig(std::string const &configItemPath) {
    // first try to get from custom config
    auto customConfigOpt = _tryGetConfig<T>(ConfigType::kCustom, configItemPath);
    if (customConfigOpt.has_value()) {
      _logger->info("TomlConfigReader::getConfig() got custom config at {}", configItemPath);
      return customConfigOpt.value();
    }

    // fallback to default config
    auto defaultConfigOpt = _tryGetConfig<T>(ConfigType::kDefault, configItemPath);
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
  std::optional<T> _getConfig(toml::v3::parse_result const &config,
                              std::string const &configItemPath) {}

  template <class T>
  std::optional<T> _tryGetConfig(ConfigType configType, std::string const &configItemPath) {
    toml::v3::parse_result const &config =
        configType == ConfigType::kDefault ? *_defaultConfig : *_customConfig;

    if (!config.succeeded()) {
      return std::nullopt;
    }

    // if not an array, just parse the value
    if constexpr (ArrayTrait<T>::value == false) {
      return config.at_path(configItemPath).value<T>();
    }
    // caution: this else block is a must, otherwise the compiler will compile the code below, even
    // with the return statement above
    else {
      toml::array const *array = config.at_path(configItemPath).as_array();

      // check if the config item is an array
      if (!array) {
        return std::nullopt;
      }
      // check if the array size matches the expected size
      if (array->size() != ArrayTrait<T>::size) {
        _logger->error("TomlConfigReader::_tryGetConfig() array size mismatch at {}",
                       configItemPath);
        return std::nullopt;
      }

      T res;
      size_t i = 0;
      for (auto const &item : *array) {
        auto val = item.value<typename ArrayTrait<T>::type>();
        if (val == std::nullopt) {
          _logger->error("TomlConfigReader::_tryGetConfig() array has invalid value for the {}th "
                         "element at {}",
                         i, configItemPath);
          return std::nullopt;
        }
        res[i++] = val.value();
      }
      return res;
    }
  }
};