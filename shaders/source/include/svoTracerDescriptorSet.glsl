#ifndef SVO_TRACER_DESCRIPTOR_SET_GLSL
#define SVO_TRACER_DESCRIPTOR_SET_GLSL

struct G_RenderInfo {
  vec3 camPosition;
  vec3 camFront;
  vec3 camUp;
  vec3 camRight;
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
layout(binding = 5, rgba32f) uniform image2D rawImage;
layout(binding = 6, rgba8) uniform image2D renderTargetImage;
layout(binding = 7, rgba8) uniform image2D octreeVisualizationImage;

layout(std430, binding = 8) readonly buffer SceneDataBufferObject { G_SceneData data; }
sceneDataBufferObject;
layout(std430, binding = 9) readonly buffer OctreeBufferObject { uint[] octreeBuffer; };
layout(std430, binding = 10) readonly buffer PaletteBufferObject { uint[] paletteBuffer; };

#endif // SVO_TRACER_DESCRIPTOR_SET_GLSL