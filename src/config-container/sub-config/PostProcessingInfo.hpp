#pragma once

#include "glm/glm.hpp" // IWYU pragma: export

class TomlConfigReader;

struct PostProcessingInfo {
  // for temporal filter info
  float temporalAlpha{};
  float temporalPositionPhi{};

  // for spatial filter info
  int aTrousIterationCount{};
  float phiC{};
  float phiN{};
  float phiP{};
  float minPhiZ{};
  float maxPhiZ{};
  float phiZStableSampleCount{};
  bool changingLuminancePhi{};

  // taa
  bool taa{};

  // tonemapping
  float explosure{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
