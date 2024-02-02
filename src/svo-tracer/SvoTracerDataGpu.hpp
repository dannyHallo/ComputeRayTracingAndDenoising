#include "utils/incl/Glm.hpp"

#include <cstdint>

struct G_RenderInfo {
  alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camPosition;
  alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camFront;
  alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camUp;
  alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camRight;
  uint32_t swapchainWidth;
  uint32_t swapchainHeight;
  float vfov;
  uint32_t currentSample;
  float time;
};

struct G_TwickableParameters {
  uint32_t magicButton;
  uint32_t visualizeOctree;
  uint32_t beamOptimization;

  // // gradient projection
  // int bypassGradientProjection;
  // alignas(sizeof(glm::vec3::x) * 4) glm::mat4 thisMvpe;

  // // stratified filtering
  // int i;
  // int bypassStratumFiltering;

  // // temporal filtering
  // int bypassTemporalFiltering;
  // int useNormalTest;
  // float normalThrehold;
  // float blendingAlpha;
  // alignas(sizeof(glm::vec3::x) * 4) glm::mat4 lastMvpe;

  // // variance estimation
  // int bypassVarianceEstimation;
  // int skipStoppingFunctions;
  // int useTemporalVariance;
  // int kernelSize;
  // float phiGaussian;
  // float phiDepth;

  // // blur filter
  // int bypassBluring;
  // int i2;
  // int iCap;
  // int useVarianceGuidedFiltering;
  // int useGradientInDepth;
  // float phiLuminance2;
  // float phiDepth2;
  // float phiNormal2;
  // int ignoreLuminanceAtFirstIteration;
  // int changingLuminancePhi;
  // int useJittering;

  // // post processing
  // uint32_t displayType;
};

struct G_SceneData {
  uint32_t beamResolution;
  uint32_t voxelLevelCount;
};

struct SvoTracerDataGpu {
  SvoTracerDataGpu() = default;

  bool magicButton      = true;
  bool visualizeOctree  = false;
  bool beamOptimization = true;

  // bool _useGradientProjection = true;

  // bool _movingLightSource = false;
  // uint32_t _outputType    = 1; // combined, direct only, indirect only
  // float _offsetX          = 0.F;
  // float _offsetY          = 0.F;

  // // StratumFilterUniformBufferObject
  // bool _useStratumFiltering = false;

  // // TemporalFilterUniformBufferObject
  // bool _useTemporalBlend = true;
  // bool _useDepthTest     = false;
  // float _depthThreshold  = 0.07F;
  // bool _useNormalTest    = true;
  // float _normalThreshold = 0.99F;
  // float _blendingAlpha   = 0.15F;

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