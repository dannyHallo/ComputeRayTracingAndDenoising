#pragma once

#include "glm/glm.hpp"
#include "utils/vulkan.h"

#include <iostream>
#include <string>
#include <unordered_map>

namespace logger {
inline void print() { std::cout << std::endl; }
inline void print(std::string str) { std::cout << str << std::endl; }
inline void print(const char *str) { std::cout << str << std::endl; }
inline void print(const char *str, const char *str2) { std::cout << str << str2 << std::endl; }
inline void print(const int num) { std::cout << num << std::endl; }
inline void print(const size_t num) { std::cout << num << std::endl; }
inline void print(const uint32_t num) { std::cout << num << std::endl; }
inline void print(const char *str, const int num) { std::cout << str << ": " << num << std::endl; }
inline void print(const char *str, const size_t num) { std::cout << str << ": " << num << std::endl; }
inline void print(const char *str, const uint32_t num) { std::cout << str << ": " << num << std::endl; }
inline void print(glm::vec3 v) { std::cout << v.x << ", " << v.y << ", " << v.z << std::endl; }

void throwWarning(const std::string);

// throw error and EXIT
void throwError(const std::string);
void checkStep(const std::string stepName, const int resultCode);
}; // namespace logger