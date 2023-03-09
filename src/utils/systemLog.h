#pragma once

#include <iostream>

static inline void print() { std::cout << std::endl; }
static inline void print(const char *str) { std::cout << str << std::endl; }
static inline void print(const char *str, const char *str2) { std::cout << str << str2 << std::endl; }
static inline void print(const int num) { std::cout << num << std::endl; }
static inline void print(const size_t num) { std::cout << num << std::endl; }
static inline void print(const uint32_t num) { std::cout << num << std::endl; }
static inline void print(const char *str, const int num) { std::cout << str << ": " << num << std::endl; }
static inline void print(const char *str, const size_t num) { std::cout << str << ": " << num << std::endl; }
static inline void print(const char *str, const uint32_t num) { std::cout << str << ": " << num << std::endl; }