#pragma once

#include "glm/glm.hpp"
#include "spdlog/spdlog.h" // spdlog/include/spdlog/details/windows_include.h defines APIENTRY!

#include <iostream>

static const int vkResultCodes[] = {
    0,           1,           2,           3,           4,           5,           -1,          -2,          -3,
    -4,          -5,          -6,          -7,          -8,          -9,          -10,         -11,         -12,
    -13,         -1000069000, -1000072003, -1000161000, -1000257000, -1000297000, -1000000000, -1000000001, -1000001003,
    -1000001004, -1000003001, -1000011001, -1000012000, -1000023000, -1000023001, -1000023002, -1000023003, -1000023004,
    -1000023005, -1000158000, -1000174001, -1000255000, -1000268000, -1000268001, -1000268002, -1000268003, -1000338000};

static const char *vkResults[] = {"VK_SUCCESS",
                                  "VK_NOT_READY",
                                  "VK_TIMEOUT",
                                  "VK_EVENT_SET",
                                  "VK_EVENT_RESET",
                                  "VK_INCOMPLETE",
                                  "VK_ERROR_OUT_OF_HOST_MEMORY",
                                  "VK_ERROR_OUT_OF_DEVICE_MEMORY",
                                  "VK_ERROR_INITIALIZATION_FAILED",
                                  "VK_ERROR_DEVICE_LOST",
                                  "VK_ERROR_MEMORY_MAP_FAILED",
                                  "VK_ERROR_LAYER_NOT_PRESENT",
                                  "VK_ERROR_EXTENSION_NOT_PRESENT",
                                  "VK_ERROR_FEATURE_NOT_PRESENT",
                                  "VK_ERROR_INCOMPATIBLE_DRIVER",
                                  "VK_ERROR_TOO_MANY_OBJECTS",
                                  "VK_ERROR_FORMAT_NOT_SUPPORTED",
                                  "VK_ERROR_FRAGMENTED_POOL",
                                  "VK_ERROR_UNKNOWN",
                                  "VK_ERROR_OUT_OF_POOL_MEMORY",
                                  "VK_ERROR_INVALID_EXTERNAL_HANDLE",
                                  "VK_ERROR_FRAGMENTATION",
                                  "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS",
                                  "VK_PIPELINE_COMPILE_REQUIRED",
                                  "VK_ERROR_SURFACE_LOST_KHR",
                                  "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR",
                                  "VK_SUBOPTIMAL_KHR",
                                  "VK_ERROR_OUT_OF_DATE_KHR",
                                  "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR",
                                  "VK_ERROR_VALIDATION_FAILED_EXT",
                                  "VK_ERROR_INVALID_SHADER_NV",
                                  "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR",
                                  "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR",
                                  "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR",
                                  "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR",
                                  "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR",
                                  "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR",
                                  "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT",
                                  "VK_ERROR_NOT_PERMITTED_KHR",
                                  "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT",
                                  "VK_THREAD_IDLE_KHR",
                                  "VK_THREAD_DONE_KHR",
                                  "VK_OPERATION_DEFERRED_KHR",
                                  "VK_OPERATION_NOT_DEFERRED_KHR",
                                  "VK_ERROR_COMPRESSION_EXHAUSTED_EXT"};

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

inline void throwWarning(const char *err) { spdlog::warn(err); }
inline void throwError(const char *err) { spdlog::error(err); }
inline void throwErrorAndWaitForQuit(const char *err) {
  spdlog::error(err);
  std::cin.get();
  exit(0);
}
inline void throwErrorAndWaitForQuit(const char *err, const int resultCode) {
  spdlog::error(err);
  for (int i = 0; i < 45; i++) {
    if (vkResultCodes[i] == resultCode) {
      spdlog::error(vkResults[i]);
    }
  }
  std::cin.get();
  exit(0);
}