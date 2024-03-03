#pragma once

#include "utils/incl/Glm.hpp" // IWYU pragma: export

#include <cstdint> // IWYU pragma: export

// alignment rule:
// https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/app09lev1sec3.html

// basically,
// scalar types are default aligned
// two component vectors are aligned twice the size of the component type
// three and four component vectors are aligned four times the size of the component type

#define vec3 alignas(16) glm::vec3
#define uvec3 alignas(16) glm::uvec3
#define vec2 alignas(8) glm::vec2
#define mat4 alignas(16) glm::mat4
#define uvec2 alignas(8) glm::uvec2
#define bool alignas(4) bool
#define uint uint32_t

#include "shaders/include/svoTracerDataStructs.glsl" // IWYU pragma: export

#undef vec3
#undef uvec3
#undef vec2
#undef mat4
#undef uvec2
#undef bool
#undef uint

struct SvoTracerTweakingData {
  SvoTracerTweakingData() = default;

  // tweakable parameters
  bool visualizeOctree   = false;
  bool beamOptimization  = true;
  bool traceSecondaryRay = true;
  bool taa               = true;

  // for env
  float sunAngle     = 0.0F;
  glm::vec3 sunColor = glm::vec3(255.F / 255.F, 208.F / 255.F, 173.F / 255.F);
  float sunLuminance = 1.F;
  float sunSize      = 0.5F;

  // for temporal filter info
  float temporalAlpha       = 0.05F;
  float temporalPositionPhi = 0.99F;

  // for spatial filter info
  int aTrousIterationCount             = 3;
  bool useVarianceGuidedFiltering      = true;
  bool useGradientInDepth              = true;
  float phiC                           = 0.01F;
  float phiN                           = 20.F;
  float phiP                           = 0.01F;
  float phiZ                           = 0.1F;
  bool ignoreLuminanceAtFirstIteration = true;
  bool changingLuminancePhi            = true;
  bool useJittering                    = false;

  // bool _useStratumFiltering = false;
  // bool _useDepthTest     = false;
  // float _depthThreshold  = 0.07F;

  // bool _useGradientProjection = true;

  // bool _movingLightSource = false;
  // uint32_t _outputType    = 1; // combined, direct only, indirect only
  // float _offsetX          = 0.F;
  // float _offsetY          = 0.F;

  // // VarianceUniformBufferObject
  // bool _useVarianceEstimation = true;
  // bool _skipStoppingFunctions = false;
  // bool _useTemporalVariance   = true;
  // int _varianceKernelSize     = 4;
  // float _variancePhiGaussian  = 1.F;
  // float _variancePhiDepth     = 0.2F;

  // // PostProcessingUniformBufferObject
  // uint32_t _displayType = 0;
};