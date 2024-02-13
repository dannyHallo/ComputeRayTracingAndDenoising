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

struct G_EnvironmentInfo {
  float sunAngle;
  vec3 sunColor;
  float sunLuminance;
  float sunSize;
};

struct G_TwickableParameters {
  bool visualizeOctree;
  bool beamOptimization;
  bool traceSecondaryRay;
};

struct G_SceneInfo {
  uint beamResolution;
  uint voxelLevelCount;
};

struct G_TemporalFilterInfo {
  float temporalAlpha;
  float temporalPositionPhi;
};

struct G_SpatialFilterInfo {
  uint aTrousIterationCount;
  bool useVarianceGuidedFiltering;
  bool useGradientInDepth;
  float phiC;
  float phiN;
  float phiP;
  float phiZ;
  bool ignoreLuminanceAtFirstIteration;
  bool changingLuminancePhi;
  bool useJittering;
};

#endif // SVO_TRACER_DATA_STRUCTS_GLSL