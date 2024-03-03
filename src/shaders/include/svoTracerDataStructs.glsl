#ifndef SVO_TRACER_DATA_STRUCTS_GLSL
#define SVO_TRACER_DATA_STRUCTS_GLSL

struct G_RenderInfo {
  vec3 camPosition;
  vec3 camFront;
  vec3 camUp;
  vec3 camRight;
  vec2 subpixOffset;
  mat4 vMat;
  mat4 vMatInv;
  mat4 vMatPrev;
  mat4 vMatPrevInv;
  mat4 pMat;
  mat4 pMatInv;
  mat4 pMatPrev;
  mat4 pMatPrevInv;
  mat4 vpMat;
  mat4 vpMatInv;
  mat4 vpMatPrev;
  mat4 vpMatPrevInv;
  uvec2 lowResSize;
  vec2 lowResSizeInv;
  uvec2 highResSize;
  vec2 highResSizeInv;
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
  bool taa;
};

struct G_SceneInfo {
  uint beamResolution;
  uint voxelLevelCount;
  uvec3 chunksDim;
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