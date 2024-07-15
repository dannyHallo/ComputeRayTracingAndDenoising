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
  uvec2 midResSize;
  vec2 midResSizeInv;
  uvec2 highResSize;
  vec2 highResSizeInv;
  float vfov;
  uint currentSample;
  float time;
};

struct G_EnvironmentInfo {
  float sunAngleA;
  float sunAngleB;
  vec3 sunColor;
  float sunLuminance;
  float atmosLuminance;
  float sunSize;
};

struct G_TwickableParameters {
  uint debugB1; // bool
  float debugF1;
  uint visualizeChunks;   // bool
  uint visualizeOctree;   // bool
  uint beamOptimization;  // bool
  uint traceSecondaryRay; // bool
  uint taa;               // bool
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
  float phiC;
  float phiN;
  float phiP;
  float phiZ;
  float phiZTolerance;
  uint changingLuminancePhi; // bool
};

struct G_OutputInfo {
  vec3 midRayHitPos;
  uint midRayHit; // bool
};

#endif // SVO_TRACER_DATA_STRUCTS_GLSL