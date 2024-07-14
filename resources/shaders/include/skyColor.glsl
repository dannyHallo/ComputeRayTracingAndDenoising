#ifndef SKY_COLOR_GLSL
#define SKY_COLOR_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/atmosCommon.glsl"
#include "../include/core/postProcessing.glsl"
#include "../include/random.glsl"

const float kSunAngleReal    = 0.01;
const float kCosSunAngleReal = cos(kSunAngleReal);
const float kTanSunAngleReal = tan(kSunAngleReal);

vec3 getRandomShadowRay(vec3 sunDir, uvec3 seed, BaseDisturbance baseDisturbance) {
  vec3 randomInUnitSphere = randomInUnitSphere(seed, baseDisturbance);
  return normalize(sunDir + randomInUnitSphere * kTanSunAngleReal);
}

bool hitSun(vec3 rayDir, vec3 sunDir) { return dot(rayDir, sunDir) >= kCosSunAngleReal; }

float _sunLuminance(vec3 rayDir, vec3 sunDir) {
  const float dropoffFac    = 600.0;
  const float dropoffPreset = 0.02; // this should be fixed
  const float cutoff        = 5e-3;

  float cosTheta = dot(rayDir, sunDir);
  if (cosTheta >= kCosSunAngleReal) {
    return 1.0;
  }

  // calculate dropoff
  float offset = kCosSunAngleReal - cosTheta;

  float lum = 1.0 / (dropoffPreset + offset * dropoffFac) * dropoffPreset;
  return max(0.0, lum - cutoff);
}

vec3 _sunColor(vec3 rayDir, vec3 sunDir) {
  float lum = _sunLuminance(rayDir, sunDir) * twickableParametersUbo.data.debugF1 * 10.0;

  if (lum == 0.0) {
    return vec3(0.0);
  }

  if (rayIntersectSphere(kCamPos, rayDir, kGroundRadiusMm) >= 0.0) {
    return vec3(0.0);
  }
  vec2 tLutUv = getLookupUv1(kCamPos, sunDir);
  return lum * textureLod(transmittanceLutTexture, tLutUv, 0).rgb;
}

vec3 skyColor(vec3 rayDir, vec3 sunDir, bool isSunAdded) {
  vec2 skyLutUv = getLookupUv2(rayDir, sunDir);
  vec3 lum      = textureLod(skyViewLutTexture, skyLutUv, 0).rgb;

  if (isSunAdded) {
    lum += _sunColor(rayDir, sunDir);
  }

  return lum;
}

#endif // SKY_COLOR_GLSL
