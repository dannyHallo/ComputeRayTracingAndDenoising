#pragma once

#include <cstdint>

enum BlockState : uint32_t {
  kRebuildSvoBuildingPipelines = 1U,
  kRebuildSvoTracingPipelines  = 2U,
  kWindowResized               = 4U,
};
