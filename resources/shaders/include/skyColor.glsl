#ifndef SKY_COLOR_GLSL
#define SKY_COLOR_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/atmosCommon.glsl"
#include "../include/core/postProcessing.glsl"

float _sunLuminance(vec3 rayDir, vec3 sunDir) {
  const float sunAngleReal  = 0.01;
  const float dropoffFac    = 600.0;
  const float dropoffPreset = 0.02; // this should be fixed
  const float cutoff        = 5e-3;

  const float cosSunAngleReal = cos(sunAngleReal);

  float cosTheta = dot(rayDir, sunDir);

  // without dropoff
  if (cosTheta >= cosSunAngleReal) {
    return 1.0;
  }

  // calculate dropoff
  float offset = cosSunAngleReal - cosTheta;

  float lum = 1.0 / (dropoffPreset + offset * dropoffFac) * dropoffPreset;
  return max(0.0, lum - cutoff);
}

vec3 _sunColor(vec3 rayDir, vec3 sunDir) {
  float lum = _sunLuminance(rayDir, sunDir);

  if (lum == 0.0) {
    return vec3(0.0);
  }

  if (rayIntersectSphere(kCamPos, rayDir, kGroundRadiusMm) >= 0.0) {
    return vec3(0.0);
  }
  vec2 tLutUv = getLookupUv1(kCamPos, sunDir);
  return lum * textureLod(transmittanceLutTexture, tLutUv, 0).rgb;
}

vec3 skyColor(vec3 rayDir, vec3 sunDir) {
  vec2 skyLutUv = getLookupUv2(rayDir, sunDir);
  vec3 lum      = textureLod(skyViewLutTexture, skyLutUv, 0).rgb;

  lum += _sunColor(rayDir, sunDir);

  // tonemapping and gamma. ScamUper ad-hoc, probably a better way to do this
  // lum *= 15.0;
  // lum = pow(lum, vec3(1.3));
  // lum /= (smoothstep(0.0, 0.2, clamp(sunDir.y, 0.0, 1.0)) * 2.0 + 0.15);

  // lum = jodieReinhardTonemap(lum);
  return lum;
}

#endif // SKY_COLOR_GLSL