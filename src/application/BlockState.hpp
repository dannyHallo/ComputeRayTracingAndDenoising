#pragma once

#include <cstdint>

enum BlockState : uint32_t {
  kShaderChanged = 1U,
  kWindowResized = 2U,
};
