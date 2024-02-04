#ifndef SVO_TRACER_DESCRIPTOR_SET_GLSL
#define SVO_TRACER_DESCRIPTOR_SET_GLSL

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
  float temporalAlpha;
};

struct G_SceneData {
  uint beamResolution;
  uint voxelLevelCount;
};

layout(binding = 0) uniform RenderInfoUbo { G_RenderInfo data; }
renderInfoUbo;

layout(binding = 1) uniform TwickableParametersUbo { G_TwickableParameters data; }
twickableParametersUbo;

layout(binding = 2, rgba8) readonly uniform image2DArray vec2BlueNoise;
layout(binding = 3, rgba8) readonly uniform image2DArray weightedCosineBlueNoise;

layout(binding = 4, r32f) uniform image2D beamDepthImage;
layout(binding = 5, rgba8) uniform image2D rawImage;
layout(binding = 6, rgba32f) uniform image2D positionImage;
layout(binding = 7, rgba8) uniform image2D octreeVisualizationImage;
layout(binding = 8, rgba32f) uniform image2D normalImage;
layout(binding = 9, rgba32f) readonly uniform image2D lastNormalImage;
layout(binding = 10, r32ui) uniform uimage2D voxHashImage;
layout(binding = 11, r32ui) readonly uniform uimage2D lastVoxHashImage;
layout(binding = 12, rgba32f) uniform image2D accumedImage;
layout(binding = 13, rgba32f) readonly uniform image2D lastAccumedImage;
layout(binding = 14, rgba8) uniform image2D varianceHistImage;
layout(binding = 15, rgba8) readonly uniform image2D lastVarianceHistImage;
layout(binding = 16, rgba8) uniform image2D renderTargetImage;

layout(std430, binding = 17) readonly buffer SceneDataBufferObject { G_SceneData data; }
sceneDataBufferObject;
layout(std430, binding = 18) readonly buffer OctreeBufferObject { uint[] octreeBuffer; };
layout(std430, binding = 19) readonly buffer PaletteBufferObject { uint[] paletteBuffer; };

#endif // SVO_TRACER_DESCRIPTOR_SET_GLSL