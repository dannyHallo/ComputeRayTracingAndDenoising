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
  uint magicButton;
  uint visualizeOctree;
  uint beamOptimization;
  uint traceSecondaryRay;
  float temporalAlpha;
};

struct G_SceneData {
  uint beamResolution;
  uint voxelLevelCount;
};

#endif // SVO_TRACER_DATA_STRUCTS_GLSL