#include "TracingInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void TracingInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  visualizeChunks  = tomlConfigReader->getConfig<bool>("TracingInfo.visualizeChunks");
  visualizeOctree  = tomlConfigReader->getConfig<bool>("TracingInfo.visualizeOctree");
  beamOptimization = tomlConfigReader->getConfig<bool>("TracingInfo.beamOptimization");
  traceIndirectRay = tomlConfigReader->getConfig<bool>("TracingInfo.traceIndirectRay");
}
