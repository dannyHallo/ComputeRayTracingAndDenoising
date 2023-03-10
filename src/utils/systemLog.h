#pragma once

#include "glm/glm.hpp"
#include <iostream>

inline void print() { std::cout << std::endl; }
inline void print(const char *str) { std::cout << str << std::endl; }
inline void print(const char *str, const char *str2) { std::cout << str << str2 << std::endl; }
inline void print(const int num) { std::cout << num << std::endl; }
inline void print(const size_t num) { std::cout << num << std::endl; }
inline void print(const uint32_t num) { std::cout << num << std::endl; }
inline void print(const char *str, const int num) { std::cout << str << ": " << num << std::endl; }
inline void print(const char *str, const size_t num) { std::cout << str << ": " << num << std::endl; }
inline void print(const char *str, const uint32_t num) { std::cout << str << ": " << num << std::endl; }
inline void print(glm::vec3 v) { std::cout << v.x << ", " << v.y << ", " << v.z << std::endl; }