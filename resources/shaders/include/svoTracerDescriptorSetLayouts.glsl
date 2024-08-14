#ifndef SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL
#define SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL

#extension GL_EXT_shader_image_load_formatted : require

#include "../include/svoTracerDataStructs.glsl"

layout(binding = 0) uniform RenderInfoUniformBuffer { G_RenderInfo data; }
renderInfoUbo;
layout(binding = 1) uniform EnvironmentUniformBuffer { G_EnvironmentInfo data; }
environmentUbo;
layout(binding = 2) uniform TweakableParametersUniformBuffer { G_TweakableParameters data; }
tweakableParametersUbo;
layout(binding = 3) uniform TemporalFilterInfoUniformBuffer { G_TemporalFilterInfo data; }
temporalFilterInfoUbo;
layout(binding = 4) uniform SpatialFilterInfoUniformBuffer { G_SpatialFilterInfo data; }
spatialFilterInfoUbo;

layout(binding = 5) readonly uniform image2DArray scalarBlueNoise;
layout(binding = 6) readonly uniform image2DArray vec2BlueNoise;
layout(binding = 7) readonly uniform image2DArray vec3BlueNoise;
layout(binding = 8) readonly uniform image2DArray weightedCosineBlueNoise;

layout(std430, binding = 9) readonly buffer ChunkIndicesBuffer { uint[] data; }
chunkIndicesBuffer;

layout(binding = 10) uniform uimage2D backgroundImage;
layout(binding = 11) uniform image2D beamDepthImage;
layout(binding = 12) uniform uimage2D rawImage;
layout(binding = 13) uniform image2D instantImage;
layout(binding = 14) uniform image2D depthImage;
layout(binding = 15) uniform image2D octreeVisualizationImage;
layout(binding = 16) uniform uimage2D hitImage;
layout(binding = 17) uniform uimage2D temporalHistLengthImage;
layout(binding = 18) uniform image2D motionImage;
layout(binding = 19) uniform uimage2D normalImage;
layout(binding = 20) readonly uniform uimage2D lastNormalImage;
layout(binding = 21) uniform image2D positionImage;
layout(binding = 22) uniform image2D lastPositionImage;
layout(binding = 23) uniform uimage2D voxHashImage;
layout(binding = 24) readonly uniform uimage2D lastVoxHashImage;
layout(binding = 25) uniform uimage2D accumedImage;
layout(binding = 26) readonly uniform uimage2D lastAccumedImage;
layout(binding = 27) uniform uimage2D godRayAccumImage;
layout(binding = 28) readonly uniform uimage2D lastGodRayAccumedImage;

// these are not uint encoded because they need to be SAMPLED as textures, see lastTaaTexture
layout(binding = 29) uniform image2D taaImage;
layout(binding = 30) uniform image2D lastTaaImage;

layout(binding = 31) uniform uimage2D blittedImage;

layout(binding = 32) uniform image2D aTrousPingImage;
layout(binding = 33) uniform image2D aTrousPongImage;

layout(binding = 34) uniform image2D renderTargetImage;

layout(binding = 35) uniform sampler2D lastTaaTexture;

layout(binding = 36) uniform image2D transmittanceLutImage;
layout(binding = 37) uniform image2D multiScatteringLutImage;
layout(binding = 38) uniform image2D skyViewLutImage;
// corresponding sampler for the above images
layout(binding = 39) uniform sampler2D transmittanceLutTexture;
layout(binding = 40) uniform sampler2D multiScatteringLutTexture;
layout(binding = 41) uniform sampler2D skyViewLutTexture;

layout(binding = 42) uniform image2D shadowMapImage;
layout(binding = 43) uniform sampler2D shadowMapTexture;

layout(std430, binding = 44) readonly buffer SceneInfoBuffer { G_SceneInfo data; }
sceneInfoBuffer;
layout(std430, binding = 45) readonly buffer OctreeBuffer { uint[] data; }
octreeBuffer;
layout(binding = 46) readonly buffer ATrousIterationBuffer { uint data; }
aTrousIterationBuffer;
layout(binding = 47) buffer OutputInfoBuffer { G_OutputInfo data; }
outputInfoBuffer;

#endif // SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL