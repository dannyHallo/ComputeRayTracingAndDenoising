#include "utils/incl/Glm.hpp"

#include <cstdint>

// alignment rule:
// https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/app09lev1sec3.html
#define vec3 alignas(16) glm::vec3
#define mat4 alignas(16) glm::mat4
#define bool alignas(4) bool
#define uint uint32_t

#include "shaders/source/include/svoTracerDataStructs.glsl"

#undef vec3
#undef mat4
#undef uint
#undef bool

struct SvoTracerTweakingData {
  SvoTracerTweakingData() = default;

  // for general info
  bool magicButton       = true;
  bool visualizeOctree   = false;
  bool beamOptimization  = true;
  bool traceSecondaryRay = true;

  // for temporal filter info
  float temporalAlpha       = 0.15F;
  float temporalPositionPhi = 0.95F;

  // for spatial filter info
  int aTrousIterationCount             = 0;
  bool useVarianceGuidedFiltering      = true;
  bool useGradientInDepth              = true;
  float phiLuminance                   = 0.3F;
  float phiDepth                       = 0.01F;
  float phiNormal                      = 128.F;
  bool ignoreLuminanceAtFirstIteration = true;
  bool changingLuminancePhi            = true;
  bool useJittering                    = true;

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