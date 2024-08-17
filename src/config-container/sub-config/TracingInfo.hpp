#pragma once

#include "glm/glm.hpp" // IWYU pragma: export

class TomlConfigReader;

struct TracingInfo {
  bool visualizeChunks{};
  bool visualizeOctree{};
  bool beamOptimization{};
  bool traceIndirectRay{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
