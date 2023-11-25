#pragma once

#include "glm/glm.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "utils/Vulkan.hpp"

#include <iostream>
#include <string>
#include <unordered_map>

#pragma once

class Logger {
  std::shared_ptr<spdlog::logger> logger;
  std::shared_ptr<spdlog::logger> printlnLogger;

public:
  Logger() {
    // the normal logger is designed to showcase the log level
    logger = spdlog::stdout_color_mt("normalLogger");
    logger->set_pattern("[%l] %v");

    // the println logger is designed to print without any log level
    printlnLogger = spdlog::stdout_color_mt("printlnLogger");
    printlnLogger->set_pattern("%v");
  }

  // print glm::vec3
  inline void print(const glm::vec3 &v) { logger->info("{}, {}, {}", v.x, v.y, v.z); }

  template <typename... Args> inline void printSubinfo(std::string format, Args &&...args) {
    std::string formatWithSubInfo = "* " + std::move(format);
    logger->info(fmt::runtime(formatWithSubInfo), args...);
  }

  // print with a format string and a variable number of arguments
  template <typename... Args> inline void print(const std::string &format, Args &&...args) {
    logger->info(fmt::runtime(format), args...);
  }

  inline void println() { printlnLogger->info(""); }

  // print a single argument
  template <typename T> inline void print(const T &t) { logger->info("{}", t); }

  // throw a warning
  inline void throwWarning(const std::string &warning) { logger->warn("Warning: {}", warning); }

  // throw an error and exit
  inline void throwError(const std::string &error) {
    logger->error("Error: {}", error);
    exit(1);
  }

  // check a step's result code
  void checkStep(const std::string stepName, const int resultCode);
};
