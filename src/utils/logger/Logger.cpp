#include "Logger.hpp" // IWYU pragma: export

#include "spdlog/sinks/stdout_color_sinks.h"

Logger::Logger() {
  // the normal logger is designed to showcase the log level
  _spdLogger = spdlog::stdout_color_mt("normalLogger");
  // %^ marks the beginning of the colorized section
  // %l will be replaced by the current log level
  // %$ marks the end of the colorized section
  _spdLogger->set_pattern("%^[%l]%$ %v");

  // the println logger is designed to print without any log level
  _printlnSpdLogger = spdlog::stdout_color_mt("printlnLogger");
  _printlnSpdLogger->set_pattern("%v");
}

Logger::~Logger() { spdlog::drop_all(); }
