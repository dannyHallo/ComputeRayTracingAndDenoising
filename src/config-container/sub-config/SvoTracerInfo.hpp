#pragma once

#include <cstdint>

class TomlConfigReader;

struct SvoTracerInfo {
  uint32_t aTrousSizeMax{};
  uint32_t beamResolution{};
  uint32_t taaSamplingOffsetSize{};
  uint32_t shadowMapResolution{};
  float upscaleRatio{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
