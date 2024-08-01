#include "SvoBuilderInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void SvoBuilderInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  chunkVoxelDim = tomlConfigReader->getConfig<uint32_t>("SvoBuilder.chunkVoxelDim");

  auto cd  = tomlConfigReader->getConfig<std::array<uint32_t, 3>>("SvoBuilder.chunkDim");
  chunksDim = glm::vec3(cd.at(0), cd.at(1), cd.at(2));
}
