#pragma once

#include "glm/glm.hpp" // IWYU pragma: export

class TomlConfigReader;

struct SvoTracerTweakingData {
  SvoTracerTweakingData(TomlConfigReader *tomlConfigReader);
  // debug parameters
  bool debugB1{};
  float debugF1{};

  // tweakable parameters
  bool visualizeChunks{};
  bool visualizeOctree{};
  bool beamOptimization{};
  bool traceSecondaryRay{};
  bool taa{};

  // for env
  float sunAngleA{};
  float sunAngleB{};
  glm::vec3 sunColor{};
  float sunLuminance{};
  float sunSize{};

  // for temporal filter info
  float temporalAlpha{};
  float temporalPositionPhi{};

  // for spatial filter info
  int aTrousIterationCount{};
  float phiC{};
  float phiN{};
  float phiP{};
  float phiZ{};
  float phiZTolerance{};
  bool changingLuminancePhi{};

private:
  TomlConfigReader *_tomlConfigReader;

  void _loadConfig();
};
