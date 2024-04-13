#ifndef SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL
#define SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL

#include "../include/svoTracerDataStructs.glsl"

layout(binding = 0) uniform RenderInfoUniformBuffer { G_RenderInfo data; }
renderInfoUbo;
layout(binding = 1) uniform EnvironmentUniformBuffer { G_EnvironmentInfo data; }
environmentUbo;
layout(binding = 2) uniform TwickableParametersUniformBuffer { G_TwickableParameters data; }
twickableParametersUbo;
layout(binding = 3) uniform TemporalFilterInfoUniformBuffer { G_TemporalFilterInfo data; }
temporalFilterInfoUbo;
layout(binding = 4) uniform SpatialFilterInfoUniformBuffer { G_SpatialFilterInfo data; }
spatialFilterInfoUbo;

layout(binding = 5, rgba8) readonly uniform image2DArray vec2BlueNoise;
layout(binding = 6, rgba8) readonly uniform image2DArray weightedCosineBlueNoise;

layout(binding = 7, r32ui) uniform uimage3D chunksImage;

layout(binding = 8, r32ui) uniform uimage2D backgroundImage;
layout(binding = 9, r32f) uniform image2D beamDepthImage;
layout(binding = 10, r32ui) uniform uimage2D rawImage;
layout(binding = 11, r32f) uniform image2D depthImage;
layout(binding = 12, r32ui) uniform uimage2D octreeVisualizationImage;
layout(binding = 13, r8ui) uniform uimage2D hitImage;
layout(binding = 14, r8ui) uniform uimage2D temporalHistLengthImage;
layout(binding = 15, rgba16f) uniform image2D motionImage;
layout(binding = 16, r32ui) uniform uimage2D normalImage;
layout(binding = 17, r32ui) readonly uniform uimage2D lastNormalImage;
layout(binding = 18, rgba32f) uniform image2D positionImage;
layout(binding = 19, rgba32f) uniform image2D lastPositionImage;
layout(binding = 20, r32ui) uniform uimage2D voxHashImage;
layout(binding = 21, r32ui) readonly uniform uimage2D lastVoxHashImage;
layout(binding = 22, r32ui) uniform uimage2D accumedImage;
layout(binding = 23, r32ui) readonly uniform uimage2D lastAccumedImage;

// these are not uint encoded because they need to be SAMPLED as textures: see binding 36
layout(binding = 24, rgba16f) uniform image2D taaImage;
layout(binding = 25, rgba16f) uniform image2D lastTaaImage;

layout(binding = 26, r32ui) uniform uimage2D blittedImage;

layout(binding = 27, r32ui) uniform uimage2D aTrousPingImage;
layout(binding = 28, r32ui) uniform uimage2D aTrousPongImage;
layout(binding = 29, r32ui) uniform uimage2D aTrousFinalResultImage;

layout(binding = 30, rgba8) uniform image2D renderTargetImage;

layout(binding = 31) uniform sampler2D lastTaaTexture;

layout(std430, binding = 32) readonly buffer SceneInfoBuffer { G_SceneInfo data; }
sceneInfoBuffer;
layout(std430, binding = 33) readonly buffer OctreeBuffer { uint[] octreeBuffer; };
layout(binding = 34) readonly buffer ATrousIterationBuffer { uint data; }
aTrousIterationBuffer;
layout(binding = 35) buffer OutputInfoBuffer { G_OutputInfo data; }
outputInfoBuffer;

#endif // SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL