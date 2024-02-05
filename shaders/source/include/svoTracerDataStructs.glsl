#ifndef SVO_TRACER_DATA_STRUCTS_GLSL
#define SVO_TRACER_DATA_STRUCTS_GLSL

struct G_RenderInfo {
  vec3 camPosition;
  vec3 camFront;
  vec3 camUp;
  vec3 camRight;
  mat4 thisMvpe;
  mat4 lastMvpe;
  uint swapchainWidth;
  uint swapchainHeight;
  float vfov;
  uint currentSample;
  float time;
};

struct G_TwickableParameters {
  bool magicButton;
  bool visualizeOctree;
  bool beamOptimization;
  bool traceSecondaryRay;
  float temporalAlpha;
};

struct G_SceneInfo {
  uint beamResolution;
  uint voxelLevelCount;
};

struct G_ATrousInfo {
  uint aTrousIterationCount;
  bool useVarianceGuidedFiltering;
  bool useGradientInDepth;
  float phiLuminance;
  float phiDepth;
  float phiNormal;
  bool ignoreLuminanceAtFirstIteration;
  bool changingLuminancePhi;
  bool useJittering;
};

#endif // SVO_TRACER_DATA_STRUCTS_GLSL