#ifndef PROJECTION_GLSL
#define PROJECTION_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

vec2 projectWorldPosToScreenUv(vec3 worldPos, bool previous) {
  vec4 screenBoxCoord = previous ? renderInfoUbo.data.vpMatPrev * vec4(worldPos, 1)
                                 : renderInfoUbo.data.vpMat * vec4(worldPos, 1);

  // points are bounded in [-1, 1] in x, y, z after the following
  screenBoxCoord /= screenBoxCoord.w;
  // points are bounded in [0, 1] in x, y, meanwhile z, w are thrown away
  screenBoxCoord = (screenBoxCoord + vec4(1.0)) * 0.5;

  vec2 screenUv = screenBoxCoord.xy;
  screenUv.y    = 1.0 - screenUv.y;

  return screenUv;
}

// projection space <-P-> view space <-V-> world space <-M-> object space
vec3 projectScreenUvToWorldCamFarPoint(vec2 screenUv, bool previous) {
  // flip y axis
  // glm's origin is at the bottom-left corner, whereas vulkan uses the top-left corner
  screenUv.y    = 1.0 - screenUv.y;
  vec4 clipPos  = vec4(screenUv * 2.0 - vec2(1.0), 1, 1);
  vec4 worldPos = previous ? (renderInfoUbo.data.vpMatPrevInv * clipPos)
                           : (renderInfoUbo.data.vpMatInv * clipPos);
  worldPos /= worldPos.w;
  return worldPos.xyz;
}

#endif // PROJECTION_GLSL