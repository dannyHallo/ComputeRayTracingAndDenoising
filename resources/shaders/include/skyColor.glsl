#ifndef SKY_COLOR_GLSL
#define SKY_COLOR_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/atmosCommon.glsl"
#include "../include/core/postProcessing.glsl"
#include "../include/random.glsl"

const float kSunAngleReal    = 0.016;
const float kCosSunAngleReal = cos(kSunAngleReal);
const float kTanSunAngleReal = tan(kSunAngleReal);

vec3 getRandomShadowRay(uvec3 seed) {
  vec3 randInSphere = randomPointInSphere(seed);
  return normalize(atmosInfoUbo.data.sunDir + randInSphere * kTanSunAngleReal);
}

float _sunLuminance(vec3 rayDir) {
  const float dropoffFac    = 20000.0;
  const float dropoffPreset = 0.02; // this should be a fixed value
  const float cutoff        = 5e-3;

  float cosTheta = dot(rayDir, atmosInfoUbo.data.sunDir);
  // if (cosTheta >= kCosSunAngleReal) {
  //   return 1.0;
  // }

  // // calculate dropoff
  // float offset = kCosSunAngleReal - cosTheta;

  // float lum = 1.0 / (dropoffPreset + offset * dropoffFac) * dropoffPreset;
  // return max(0.0, lum - cutoff);

  if (cosTheta >= kCosSunAngleReal) {
    return 1.0;
  }
  return 0.0;
}

vec3 _sunColor(vec3 rayDir) {
  float lum = _sunLuminance(rayDir);

  if (lum == 0.0) {
    return vec3(0.0);
  }

  if (rayIntersectSphere(kCamPos, rayDir, kGroundRadiusMm) >= 0.0) {
    return vec3(0.0);
  }
  vec2 tLutUv = getLookupUv1(kCamPos, atmosInfoUbo.data.sunDir);
  return lum * textureLod(transmittanceLutTexture, tLutUv, 0).rgb;
}

vec3 skyColor(vec3 rayDir, bool isSunAdded) {
  vec2 skyLutUv = getLookupUv2(rayDir, atmosInfoUbo.data.sunDir);
  vec3 atmosCol = textureLod(skyViewLutTexture, skyLutUv, 0).rgb;
  atmosCol *= 30.0 * atmosInfoUbo.data.atmosLuminance;

  vec3 sunCol = vec3(0.0);
  if (isSunAdded) {
    sunCol = _sunColor(rayDir) * atmosInfoUbo.data.sunLuminance * 10000.0;
  }

  vec3 skyCol = pow(atmosCol + sunCol, vec3(1.3));

  // skyCol /= (smoothstep(0.0, 0.2, clamp(sunDir.y, 0.0, 1.0)) * 2.0 + 0.15);

  return skyCol;
}

#endif // SKY_COLOR_GLSL
