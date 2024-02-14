#ifndef PROJECTION_GLSL
#define PROJECTION_GLSL

#include "include/svoTracerDescriptorSetLayouts.glsl"

// Projection space <-P-> View space <-V-> World space <-M-> Object space
vec3 projectScreenUvToWorldCamFarPoint(vec2 screenUv, bool previous) {
  vec4 clipPos  = vec4(screenUv * 2.0 - vec2(1.0), 1, 1);
  vec4 worldPos = previous
                      ? (renderInfoUbo.data.vMatPrevInv * renderInfoUbo.data.pMatPrevInv * clipPos)
                      : (renderInfoUbo.data.vMatInv * renderInfoUbo.data.pMatInv * clipPos);
  worldPos /= worldPos.w;
  return worldPos.xyz;
}

#endif // PROJECTION_GLSL