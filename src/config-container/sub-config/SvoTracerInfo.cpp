#include "SvoTracerInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void SvoTracerInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  aTrousSizeMax         = tomlConfigReader->getConfig<uint32_t>("SvoTracer.aTrousSizeMax");
  beamResolution        = tomlConfigReader->getConfig<uint32_t>("SvoTracer.beamResolution");
  taaSamplingOffsetSize = tomlConfigReader->getConfig<uint32_t>("SvoTracer.taaSamplingOffsetSize");
  shadowMapResolution   = tomlConfigReader->getConfig<uint32_t>("SvoTracer.shadowMapResolution");
  upscaleRatio          = tomlConfigReader->getConfig<float>("SvoTracer.upscaleRatio");
}
