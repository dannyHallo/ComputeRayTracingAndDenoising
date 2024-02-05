#ifndef SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL
#define SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL

#include "svoTracerDataStructs.glsl"

layout(binding = 0) uniform RenderInfoUniformBuffer { G_RenderInfo data; }
renderInfoUbo;

layout(binding = 1) uniform TwickableParametersUniformBuffer { G_TwickableParameters data; }
twickableParametersUbo;

layout(binding = 23) uniform ATrousInfoUniformBuffer { G_ATrousInfo data; }
aTrousInfoUbo;

layout(binding = 2, rgba8) readonly uniform image2DArray vec2BlueNoise;
layout(binding = 3, rgba8) readonly uniform image2DArray weightedCosineBlueNoise;

layout(binding = 4, r32f) uniform image2D beamDepthImage;
layout(binding = 5, rgba8) uniform image2D rawImage;
layout(binding = 6, rgba8) uniform image2D depthImage;
layout(binding = 7, rgba32f) uniform image2D positionImage;
layout(binding = 8, rgba8) uniform image2D octreeVisualizationImage;
layout(binding = 9, rgba32f) uniform image2D normalImage;
layout(binding = 10, rgba32f) readonly uniform image2D lastNormalImage;
layout(binding = 11, r32ui) uniform uimage2D voxHashImage;
layout(binding = 12, r32ui) readonly uniform uimage2D lastVoxHashImage;
layout(binding = 13, rgba8) uniform image2D accumedImage;
layout(binding = 14, rgba8) readonly uniform image2D lastAccumedImage;
layout(binding = 15, rgba32f) uniform image2D varianceHistImage;
layout(binding = 16, rgba32f) readonly uniform image2D lastVarianceHistImage;
layout(binding = 17, rgba8) uniform image2D aTrousPingImage;
layout(binding = 18, rgba8) uniform image2D aTrousPongImage;
layout(binding = 25, rgba8) uniform image2D aTrousFinalResultImage;
layout(binding = 19, rgba8) uniform image2D renderTargetImage;

layout(std430, binding = 20) readonly buffer SceneDataBuffer { G_SceneInfo data; }
sceneDataBuffer;
layout(std430, binding = 21) readonly buffer OctreeBuffer { uint[] octreeBuffer; };
layout(std430, binding = 22) readonly buffer PaletteBuffer { uint[] paletteBuffer; };
layout(binding = 24) readonly buffer ATrousIterationBuffer { uint data; }
aTrousIterationBuffer;

#endif // SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL