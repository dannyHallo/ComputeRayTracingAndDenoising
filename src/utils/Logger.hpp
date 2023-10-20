#pragma once

#include "glm/glm.hpp"
#include "spdlog/spdlog.h"
#include "utils/Vulkan.hpp"

#include <iostream>
#include <string>
#include <unordered_map>

#pragma once

namespace Logger {

// Print glm::vec3 directly
inline void print(const glm::vec3 &v) {
  spdlog::info("{}, {}, {}", v.x, v.y, v.z);
}

// Print with a format string and a variable number of arguments
template <typename... Args>
inline void print(const std::string &format, Args &&...args) {
  spdlog::info(fmt::runtime(format), args...);
}

// Print a single argument
template <typename T> inline void print(const T &t) { spdlog::info("{}", t); }

// Throw a warning
inline void throwWarning(const std::string &warning) {
  spdlog::warn("Warning: {}", warning);
}

// Throw an error and exit
inline void throwError(const std::string &error) {
  spdlog::error("Error: {}", error);
  exit(1);
}

// Check a step's result code
void checkStep(const std::string stepName, const int resultCode);
}; // namespace Logger