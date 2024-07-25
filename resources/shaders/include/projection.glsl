#ifndef PROJECTION_GLSL
#define PROJECTION_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

vec2 projectWorldPosToScreenUv(vec3 worldPos, bool previous) {
  vec4 screenBoxCoord = previous ? renderInfoUbo.data.vpMatPrev * vec4(worldPos, 1.0)
                                 : renderInfoUbo.data.vpMat * vec4(worldPos, 1.0);

  // points are bounded in [-1, 1] for x, y after the following, z should be in [0, 1]
  screenBoxCoord /= screenBoxCoord.w;
  vec2 screenUv = screenBoxCoord.xy;

  // points are bounded in [0, 1] for x, y
  screenUv   = (screenUv + 1.0) * 0.5;
  screenUv.y = 1.0 - screenUv.y;
  return screenUv;
}

// projection space <-P-> view space <-V-> world space <-M-> object space
vec3 projectScreenUvToWorldCamFarPoint(vec2 screenUv, bool previous) {
  // flip y axis
  // glm's origin is at the bottom-left corner, whereas vulkan uses the top-left corner
  screenUv.y    = 1.0 - screenUv.y;
  vec4 clipPos  = vec4(screenUv * 2.0 - 1.0, 1, 1);
  vec4 worldPos = previous ? (renderInfoUbo.data.vpMatPrevInv * clipPos)
                           : (renderInfoUbo.data.vpMatInv * clipPos);
  worldPos /= worldPos.w;
  return worldPos.xyz;
}

// projection space <-P-> view space <-V-> world space <-M-> object space
vec3 projectScreenUvToWorldCamFarPointShadowMap(vec2 screenUv) {
  // flip y axis
  // glm's origin is at the bottom-left corner, whereas vulkan uses the top-left corner
  screenUv.y    = 1.0 - screenUv.y;
  vec4 clipPos  = vec4(screenUv * 2.0 - 1.0, 1.0, 1.0);
  vec4 worldPos = renderInfoUbo.data.vpMatShadowMapCamInv * clipPos;
  worldPos /= worldPos.w;
  return worldPos.xyz;
}

// projection space <-P-> view space <-V-> world space <-M-> object space
vec3 projectShadowMapUvToShadowMapCamNearPoint(vec2 shadowMapUv) {
  // flip y axis
  // glm's origin is at the bottom-left corner, whereas vulkan uses the top-left corner
  shadowMapUv.y     = 1.0 - shadowMapUv.y;
  vec4 clipPos      = vec4(shadowMapUv * 2.0 - 1.0, 0.0, 1.0);
  vec4 worldPosNear = renderInfoUbo.data.vpMatShadowMapCamInv * clipPos;
  worldPosNear /= worldPosNear.w;
  return worldPosNear.xyz;
}

vec2 projectWorldPosToShadowMapUv(vec3 worldPos) {
  vec4 screenBoxCoord = renderInfoUbo.data.vpMatShadowMapCam * vec4(worldPos, 1.0);
  screenBoxCoord /= screenBoxCoord.w;
  vec2 shadowMapUv = screenBoxCoord.xy;
  shadowMapUv      = (shadowMapUv + 1.0) * 0.5;
  shadowMapUv.y    = 1.0 - shadowMapUv.y;
  return shadowMapUv;
}

#endif // PROJECTION_GLSL
