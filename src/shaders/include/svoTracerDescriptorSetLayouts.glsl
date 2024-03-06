#ifndef SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL
#define SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL

#include "../include/svoTracerDataStructs.glsl"

layout(binding = 0) uniform RenderInfoUniformBuffer { G_RenderInfo data; }
renderInfoUbo;
layout(binding = 31) uniform EnvironmentUniformBuffer { G_EnvironmentInfo data; }
environmentUbo;
layout(binding = 1) uniform TwickableParametersUniformBuffer { G_TwickableParameters data; }
twickableParametersUbo;
layout(binding = 27) uniform TemporalFilterInfoUniformBuffer { G_TemporalFilterInfo data; }
temporalFilterInfoUbo;
layout(binding = 23) uniform SpatialFilterInfoUniformBuffer { G_SpatialFilterInfo data; }
spatialFilterInfoUbo;

layout(binding = 2, rgba8) readonly uniform image2DArray vec2BlueNoise;
layout(binding = 3, rgba8) readonly uniform image2DArray weightedCosineBlueNoise;

layout(binding = 37, r32ui) uniform uimage3D chunksImage;

layout(binding = 29, r32ui) uniform uimage2D backgroundImage;
layout(binding = 4, r32f) uniform image2D beamDepthImage;
layout(binding = 5, r32ui) uniform uimage2D rawImage;
layout(binding = 6, r32f) uniform image2D depthImage;
layout(binding = 8, r32ui) uniform uimage2D octreeVisualizationImage;
layout(binding = 28, r8ui) uniform uimage2D hitImage;
layout(binding = 30, r8ui) uniform uimage2D temporalHistLengthImage;
layout(binding = 32, rgba16f) uniform image2D motionImage;
layout(binding = 9, r32ui) uniform uimage2D normalImage;
layout(binding = 10, r32ui) readonly uniform uimage2D lastNormalImage;
layout(binding = 7, rgba32f) uniform image2D positionImage;
layout(binding = 26, rgba32f) uniform image2D lastPositionImage;
layout(binding = 11, r32ui) uniform uimage2D voxHashImage;
layout(binding = 12, r32ui) readonly uniform uimage2D lastVoxHashImage;
layout(binding = 13, r32ui) uniform uimage2D accumedImage;
layout(binding = 14, r32ui) readonly uniform uimage2D lastAccumedImage;

// these are not uint encoded because they need to be sampled
layout(binding = 33, rgba16f) uniform image2D taaImage;
layout(binding = 34, rgba16f) uniform image2D lastTaaImage;

layout(binding = 35, r32ui) uniform uimage2D blittedImage;

// layout(binding = 15, rgba32f) uniform image2D varianceHistImage;
// layout(binding = 16, rgba32f) readonly uniform image2D lastVarianceHistImage;

layout(binding = 17, r32ui) uniform uimage2D aTrousPingImage;
layout(binding = 18, r32ui) uniform uimage2D aTrousPongImage;
layout(binding = 25, r32ui) uniform uimage2D aTrousFinalResultImage;
layout(binding = 19, rgba8) uniform image2D renderTargetImage;

layout(binding = 36) uniform sampler2D lastTaaTexture;

layout(std430, binding = 20) readonly buffer SceneInfoBuffer { G_SceneInfo data; }
sceneInfoBuffer;
layout(std430, binding = 21) readonly buffer OctreeBuffer { uint[] octreeBuffer; };
// layout(std430, binding = 22) readonly buffer PaletteBuffer { uint[] paletteBuffer; };
layout(binding = 24) readonly buffer ATrousIterationBuffer { uint data; }
aTrousIterationBuffer;

#endif // SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL