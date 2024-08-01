#pragma once

#include <glm/glm.hpp>

#include <cstdint>

class TomlConfigReader;

struct TerrainInfo {
  uint32_t chunkVoxelDim{};
  glm::uvec3 chunksDim{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
