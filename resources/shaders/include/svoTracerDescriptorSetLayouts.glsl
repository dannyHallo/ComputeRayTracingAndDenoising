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

layout(binding = 48) readonly uniform image2DArray scalarBlueNoise;
layout(binding = 5) readonly uniform image2DArray vec2BlueNoise;
layout(binding = 42) readonly uniform image2DArray vec3BlueNoise;
layout(binding = 6) readonly uniform image2DArray weightedCosineBlueNoise;

layout(binding = 7) uniform uimage3D chunksImage;

layout(binding = 8) uniform uimage2D backgroundImage;
layout(binding = 9) uniform image2D beamDepthImage;
layout(binding = 10) uniform uimage2D rawImage;
layout(binding = 45) uniform uimage2D godRayImage;
layout(binding = 11) uniform image2D depthImage;
layout(binding = 12) uniform image2D octreeVisualizationImage;
layout(binding = 13) uniform uimage2D hitImage;
layout(binding = 14) uniform uimage2D temporalHistLengthImage;
layout(binding = 15) uniform image2D motionImage;
layout(binding = 16) uniform uimage2D normalImage;
layout(binding = 17) readonly uniform uimage2D lastNormalImage;
layout(binding = 18) uniform image2D positionImage;
layout(binding = 19) uniform image2D lastPositionImage;
layout(binding = 20) uniform uimage2D voxHashImage;
layout(binding = 21) readonly uniform uimage2D lastVoxHashImage;
layout(binding = 22) uniform uimage2D accumedImage;
layout(binding = 23) readonly uniform uimage2D lastAccumedImage;
layout(binding = 46) uniform uimage2D godRayAccumImage;
layout(binding = 47) readonly uniform uimage2D lastGodRayAccumedImage;

// these are not uint encoded because they need to be SAMPLED as textures, see lastTaaTexture
layout(binding = 24) uniform image2D taaImage;
layout(binding = 25) uniform image2D lastTaaImage;

layout(binding = 26) uniform uimage2D blittedImage;

layout(binding = 27) uniform image2D aTrousPingImage;
layout(binding = 28) uniform image2D aTrousPongImage;
// layout(binding = 29) uniform image2D aTrousResultImage;

layout(binding = 30) uniform image2D renderTargetImage;

layout(binding = 31) uniform sampler2D lastTaaTexture;

layout(binding = 36) uniform image2D transmittanceLutImage;
layout(binding = 37) uniform image2D multiScatteringLutImage;
layout(binding = 38) uniform image2D skyViewLutImage;
// corresponding sampler for the above images
layout(binding = 39) uniform sampler2D transmittanceLutTexture;
layout(binding = 40) uniform sampler2D multiScatteringLutTexture;
layout(binding = 41) uniform sampler2D skyViewLutTexture;

layout(binding = 43) uniform image2D shadowMapImage;
layout(binding = 44) uniform sampler2D shadowMapTexture;

layout(std430, binding = 32) readonly buffer SceneInfoBuffer { G_SceneInfo data; }
sceneInfoBuffer;
layout(std430, binding = 33) readonly buffer OctreeBuffer { uint[] octreeBuffer; };
layout(binding = 34) readonly buffer ATrousIterationBuffer { uint data; }
aTrousIterationBuffer;
layout(binding = 35) buffer OutputInfoBuffer { G_OutputInfo data; }
outputInfoBuffer;

#endif // SVO_TRACER_DESCRIPTOR_SET_LAYOUTS_GLSL