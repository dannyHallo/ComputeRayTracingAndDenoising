#pragma once

#include "glm/glm.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "utils/incl/Vulkan.hpp"

#include <iostream>
#include <string>
#include <unordered_map>

#pragma once

class Logger {
  std::shared_ptr<spdlog::logger> _spdLogger;
  std::shared_ptr<spdlog::logger> _printlnSpdLogger;

public:
  Logger() {
    // the normal logger is designed to showcase the log level
    _spdLogger = spdlog::stdout_color_mt("normalLogger");
    // %^ marks the beginning of the colorized section.
    // %l specifies that the log level should be inserted here.
    // %$ marks the end of the colorized section.
    _spdLogger->set_pattern("%^[%l] %v%$");

    // the println logger is designed to print without any log level
    _printlnSpdLogger = spdlog::stdout_color_mt("printlnLogger");
    _printlnSpdLogger->set_pattern("%v");
  }

  inline void print(const glm::vec3 &v) { _spdLogger->info("{}, {}, {}", v.x, v.y, v.z); }

  template <typename... Args> inline void subInfo(std::string format, Args &&...args) {
    std::string formatWithSubInfo = "* " + std::move(format);
    _spdLogger->info(fmt::runtime(formatWithSubInfo), args...);
  }

  template <typename... Args> inline void info(const std::string &format, Args &&...args) {
    _spdLogger->info(fmt::runtime(format), args...);
  }
  template <typename... Args> inline void warn(const std::string &format, Args &&...args) {
    _spdLogger->warn(fmt::runtime(format), args...);
  }
  template <typename... Args> inline void error(const std::string &format, Args &&...args) {
    _spdLogger->error(fmt::runtime(format), args...);
  }

  inline void println() { _printlnSpdLogger->info(""); }

  template <typename T> inline void info(const T &t) { _spdLogger->info("{}", t); }
  template <typename T> inline void warn(const T &t) { _spdLogger->warn("{}", t); }
  template <typename T> inline void error(const T &t) { _spdLogger->error("{}", t); }

  // void checkStep(const std::string stepName, const int resultCode);
};
