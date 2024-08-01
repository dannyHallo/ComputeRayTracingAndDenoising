#pragma once

#include <glm/glm.hpp>

#include <cstdint>

class TomlConfigReader;

struct SvoBuilderInfo {
  uint32_t chunkVoxelDim{};
  glm::uvec3 chunksDim{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
