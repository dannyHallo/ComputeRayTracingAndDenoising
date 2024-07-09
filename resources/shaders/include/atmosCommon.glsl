#ifndef ATMOS_COMMON_GLSL
#define ATMOS_COMMON_GLSL

#include "../include/core/definitions.glsl"

// phase function: describes the directionality of scattering.
// scattering coefficients: describe the intensity of scattering.

// units are in Megameters (1000 km = 1 Mm)
const float kOneMetreMm         = 1e-6;
const float kGroundRadiusMm     = 6.36;
const float kAtmosphereRadiusMm = 6.46;

const vec3 kCamPos = vec3(0.0, 6.3602, 0.0);

const vec2 kTLutRes   = vec2(256.0, 64.0);
const vec2 kMsLutRes  = vec2(32.0);
const vec2 kSkyLutRes = vec2(200.0);

const vec3 kGroundAlbedo = vec3(0.3);

// found in sec 4, table 1
const vec3 kRayleighScatteringBase = vec3(5.802, 13.558, 33.1);
// rayleigh does not absorb

const float kMieScatteringBase = 3.996;
const float kMieAbsorptionBase = 4.4;

// ozone does not scatter
const vec3 kOzoneAbsorptionBase = vec3(0.650, 1.881, 0.085);

// input:  [0, 1]
// output: [-pi, pi]
float getSunAltitude(float sunPos) { return (sunPos * 2.0 - 1.0) * kPi; }

// input: the angle between the sun and -z axis, using +y as the positive
// output: unit vector pointing towards the sun
vec3 getSunDir(float sunAltitude) { return vec3(0.0, sin(sunAltitude), -cos(sunAltitude)); }

// the phase function for rayleigh scattering
// theta is the angle between the incident light and the scattered light
float getRayleighPhase(float cosTheta) {
  const float k = 3.0 / (16.0 * kPi);
  return k * (1.0 + cosTheta * cosTheta);
}

// the phase function for mie scattering
float getMiePhase(float cosTheta) {
  // g is the asymmetry factor, in [-1, 1]
  // when g = 0, the phase function is isotropic (just like rayleigh scattering)
  // when g > 0, the phase function is forward-focused
  // when g < 0, the phase function is backward-focused
  const float g     = 0.8;
  const float scale = 3.0 / (8.0 * kPi);

  float num   = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
  float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

  return scale * num / denom;
}

// calculate the scattering weights and total extinction for a given position
// the proportion of light that is extincted over a small step t can be
// calculated using the formula exp(-w * t), where w is the extinction weight.
void getScatteringValues(vec3 pos, out vec3 oRayleighScattering, out float oMieScattering,
                         out vec3 oExtinction) {
  float altitudeKM = (length(pos) - kGroundRadiusMm) * 1e3;

  // the following magic values can be found in sec 4 (1 / 8 = 0.125 and 1 / 1.2
  // = 0.833)
  float rayleighDensity = exp(-altitudeKM * 0.125);
  float mieDensity      = exp(-altitudeKM * 0.833);

  oRayleighScattering = kRayleighScatteringBase * rayleighDensity;

  oMieScattering      = kMieScatteringBase * mieDensity;
  float mieAbsorption = kMieAbsorptionBase * mieDensity;

  // ozone does not scatter
  vec3 ozoneAbsorption = kOzoneAbsorptionBase * max(0.0, 1.0 - abs(altitudeKM - 25.0) / 15.0);

  // the sum of all scattering and obsorbtion
  oExtinction = oRayleighScattering + oMieScattering + mieAbsorption + ozoneAbsorption;
}

// returns: -1 - when no hitting point, or the distance to the closest hit of
// the sphere
float rayIntersectSphere(vec3 ro, vec3 rd, float radius) {
  float t  = -dot(ro, rd);
  float b2 = dot(ro, ro);
  float c  = b2 - radius * radius;

  // if rd is outside the sphere, and the ray is departuring the sphere, we can
  // apply a fast culling
  if (c > 0.0 && t < 0.0) {
    return -1.0;
  }

  float discr = t * t - c;
  // no hit at all
  if (discr < 0.0) {
    return -1.0;
  }

  float h = sqrt(discr);
  // outside sphere, use near insec point
  if (c > 0.0) {
    return t - h;
  }
  // inside sphere, use far insec point
  return t + h;
}

// used when CREATING the transmittance LUT and multi-scattering LUT
void uvToPosAndSunDir(out vec3 pos, out vec3 sunDir, vec2 uv) {
  // [-1, 1)
  float sunCosTheta = 2.0 * uv.x - 1.0;
  // the result of arccos lays in [0, pi]
  // [pi, 0)
  float sunTheta = acos(sunCosTheta);
  // [kGroundRadius, kAtmosRadius)
  float height = mix(kGroundRadiusMm + kOneMetreMm, kAtmosphereRadiusMm - kOneMetreMm, uv.y);

  pos    = vec3(0.0, height, 0.0);
  sunDir = normalize(vec3(0.0, sunCosTheta, -sin(sunTheta)));
}

// used when PARSING the transmittance LUT and multi-scattering LUT
vec2 getLookupUv1(vec3 pos, vec3 sunDir) {
  float height = length(pos);
  // the normalized up vector
  vec3 up = pos / height;
  // theta is the angle from up vector to sun vector
  float sunCosTheta = dot(sunDir, up);

  return vec2(0.5 * sunCosTheta + 0.5,
              (height - kGroundRadiusMm) / (kAtmosphereRadiusMm - kGroundRadiusMm));
}

float azimuthToUvX(float azimuth) { return (azimuth / kPi + 1.0) * 0.5; }

// see section 5.3
// input:  [-0.5pi, 0.5pi)
// output: [0, 1)
// non-linear encoding
float altitudeToUvY(float altitude) {
  return 0.5 + 0.5 * sign(altitude) * sqrt(abs(altitude) * 2.0 / kPi);
}

// used when PARSING the sky view LUT
vec2 getLookupUv2(vec3 rayDir, vec3 sunDir) {
  // the ray altitude, relative to the up vec of the cam
  // (-0.5pi, 0.5pi)
  float rayAlt = asin(rayDir.y);

  // (0, 2pi)
  float azimuth;
  // looking straight up or down
  azimuth = atan(rayDir.x, rayDir.z);
  return vec2(azimuthToUvX(azimuth), altitudeToUvY(rayAlt));
}

#endif // ATMOS_COMMON_GLSL
