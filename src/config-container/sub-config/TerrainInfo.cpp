#include "TerrainInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void TerrainInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  chunkVoxelDim  = tomlConfigReader->getConfig<uint32_t>("Terrain.chunkVoxelDim");
  auto const &cd = tomlConfigReader->getConfig<std::array<uint32_t, 3>>("Terrain.chunkDim");
  chunksDim      = glm::vec3(cd.at(0), cd.at(1), cd.at(2));
}
