#pragma once

#include "glm/glm.hpp"
#include "utils/vulkan.h"

#include <iostream>
#include <string>
#include <unordered_map>

class logger {
  static const std::unordered_map<int, std::string> resultCodeNamePair;

public:
  logger()  = default;
  ~logger() = default;

  static inline void print() { std::cout << std::endl; }
  static inline void print(const char *str) { std::cout << str << std::endl; }
  static inline void print(const char *str, const char *str2) { std::cout << str << str2 << std::endl; }
  static inline void print(const int num) { std::cout << num << std::endl; }
  static inline void print(const size_t num) { std::cout << num << std::endl; }
  static inline void print(const uint32_t num) { std::cout << num << std::endl; }
  static inline void print(const char *str, const int num) { std::cout << str << ": " << num << std::endl; }
  static inline void print(const char *str, const size_t num) { std::cout << str << ": " << num << std::endl; }
  static inline void print(const char *str, const uint32_t num) { std::cout << str << ": " << num << std::endl; }
  static inline void print(glm::vec3 v) { std::cout << v.x << ", " << v.y << ", " << v.z << std::endl; }

  // throw warning
  static void throwWarning(const std::string);
  // throw error and exit
  static void throwError(const std::string);
  static void checkStep(const std::string stepName, const int resultCode);
};