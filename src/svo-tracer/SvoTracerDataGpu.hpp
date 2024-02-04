#include "utils/incl/Glm.hpp"

#include <cstdint>

// alignment rule:
// https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/app09lev1sec3.html
#define vec3 alignas(16) glm::vec3
#define mat4 alignas(16) glm::mat4
#define uint uint32_t

#include "shaders/source/include/svoTracerDataStructs.glsl"

#undef vec3
#undef mat4
#undef uint

struct SvoTracerDataGpu {
  SvoTracerDataGpu() = default;

  bool magicButton       = true;
  bool visualizeOctree   = false;
  bool beamOptimization  = true;
  bool traceSecondaryRay = true;
  float temporalAlpha    = 0.15F;
  // bool _useDepthTest     = false;
  // float _depthThreshold  = 0.07F;

  // bool _useGradientProjection = true;

  // bool _movingLightSource = false;
  // uint32_t _outputType    = 1; // combined, direct only, indirect only
  // float _offsetX          = 0.F;
  // float _offsetY          = 0.F;

  // // StratumFilterUniformBufferObject
  // bool _useStratumFiltering = false;

  // // VarianceUniformBufferObject
  // bool _useVarianceEstimation = true;
  // bool _skipStoppingFunctions = false;
  // bool _useTemporalVariance   = true;
  // int _varianceKernelSize     = 4;
  // float _variancePhiGaussian  = 1.F;
  // float _variancePhiDepth     = 0.2F;

  // // BlurFilterUniformBufferObject
  // bool _useATrous = true;
  // int _iCap;
  // bool _useVarianceGuidedFiltering      = true;
  // bool _useGradientInDepth              = true;
  // float _phiLuminance                   = 0.3F;
  // float _phiDepth                       = 0.2F;
  // float _phiNormal                      = 128.F;
  // bool _ignoreLuminanceAtFirstIteration = true;
  // bool _changingLuminancePhi            = true;
  // bool _useJittering                    = true;

  // // PostProcessingUniformBufferObject
  // uint32_t _displayType = 0;
};