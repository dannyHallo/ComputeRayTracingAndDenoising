#pragma once

#include "fmt/format.h"    // IWYU pragma: export
#include "spdlog/spdlog.h" // IWYU pragma: export

#include <string>
#include <utility>

class Logger {
public:
  Logger();
  ~Logger();

  template <typename... Args> inline void subInfo(std::string format, Args &&...args) {
    std::string formatWithSubInfo = "* " + std::move(format);
    _spdLogger->info(fmt::runtime(formatWithSubInfo), std::forward<Args>(args)...);
  }

  template <typename... Args> inline void info(const std::string &format, Args &&...args) {
    _spdLogger->info(fmt::runtime(format), std::forward<Args>(args)...);
  }
  template <typename... Args> inline void warn(const std::string &format, Args &&...args) {
    _spdLogger->warn(fmt::runtime(format), std::forward<Args>(args)...);
  }
  template <typename... Args> inline void error(const std::string &format, Args &&...args) {
    _spdLogger->error(fmt::runtime(format), std::forward<Args>(args)...);
  }

  inline void println() { _printlnSpdLogger->info(""); }

  template <typename T> inline void info(const T &t) { _spdLogger->info("{}", t); }
  template <typename T> inline void warn(const T &t) { _spdLogger->warn("{}", t); }
  template <typename T> inline void error(const T &t) { _spdLogger->error("{}", t); }

private:
  std::shared_ptr<spdlog::logger> _spdLogger;
  std::shared_ptr<spdlog::logger> _printlnSpdLogger;
};
