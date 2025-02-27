#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/core/definitions.glsl"
#include "../include/core/geom.glsl"


// // Construct a float with half-open range [0:1] using low 23 bits.
// // All zeroes yields 0.0, all ones yields the next smallest representable value
// // below 1.0.
// float floatConstruct(uint m) {
//   const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
//   const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

//   m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
//   m |= ieeeOne;      // Add fractional part to 1.0

//   float f = uintBitsToFloat(m); // Range [1:2]
//   return f - 1.0;               // Range [0:1]
// }

// // Pseudo-random value in half-open range [0:1].
// float random(uint x) { return floatConstruct(hash(x)); }

// uint rngState = 0;
// float random(uvec3 seed) {
//   if (rngState == 0) {
//     uint index = seed.x + renderInfoUbo.data.lowResSize.x * seed.y + 1;
//     rngState   = index * renderInfoUbo.data.currentSample + 1;
//   } else {
//     rngState = hash(rngState);
//   }
//   return random(rngState);
// }

// guides to low descripancy sequence:
// http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences
// https://psychopath.io/post/2014_06_28_low_discrepancy_sequences
// https://github.com/NVIDIAGameWorks/SpatiotemporalBlueNoiseSDK?tab=readme-ov-file

// rounding off issues:
// https://stackoverflow.com/questions/32231777/how-to-avoid-rounding-off-of-large-float-or-double-values

// shaders:
// this solution avoids float rounding errors
// https://www.shadertoy.com/view/4dtBWH

const uvec3 kBlueNoiseSize = uvec3(128, 128, 64);
const float invExp         = 1 / exp2(24.);

// float goldenLds(float base, float n) { return fract(base + kGoldenRatio * n); }

vec2 _getOffsetFromDisturbance(uint disturb) {
  uint n         = disturb + 777123;
  vec2 ldsOffset = fract(ivec2(12664746 * n, 9560334 * n) * invExp);
  return ldsOffset;
}

uvec3 makeDisturbedSeed(vec3 seed, uint disturb) {
  vec2 offsetBasedOnDisturbance = _getOffsetFromDisturbance(disturb);
  return uvec3(seed.x + offsetBasedOnDisturbance.x * kBlueNoiseSize.x,
               seed.y + offsetBasedOnDisturbance.y * kBlueNoiseSize.x, seed.z);
}

ivec3 _getStbnSampleUvi(uvec3 seed) {
  return ivec3(seed.x % kBlueNoiseSize.x, seed.y % kBlueNoiseSize.y, seed.z % kBlueNoiseSize.z);
}

float stbnScalar(uvec3 seed) { return imageLoad(scalarBlueNoise, _getStbnSampleUvi(seed)).r; }
vec2 stbnVec2(uvec3 seed) { return imageLoad(vec2BlueNoise, _getStbnSampleUvi(seed)).rg; }
vec3 stbnVec3(uvec3 seed) { return imageLoad(vec3BlueNoise, _getStbnSampleUvi(seed)).rgb; }
vec3 stbnWeightedCosine(uvec3 seed) {
  return imageLoad(weightedCosineBlueNoise, _getStbnSampleUvi(seed)).rgb;
}

vec3 randomPointOnSphere(uvec3 seed) {
  vec2 rand = stbnVec2(seed);

  // (1, -1) => acos => (0, pi), uniform before acos
  float phi   = acos(1.0 - 2.0 * rand.x);
  float theta = 2.0 * kPi * rand.y;

  return getSphericalDir(theta, phi);
}

vec3 randomPointOnHemisphere(vec3 normal, uvec3 seed) {
  vec3 pointOnSphere = randomPointOnSphere(seed);
  if (dot(pointOnSphere, normal) > 0.0)
    return pointOnSphere;
  else
    return -pointOnSphere;
}

vec3 randomPointInSphere(uvec3 seed) {
  vec3 rand = stbnVec3(seed);

  // (1, -1) => acos => (0, pi), uniform before acos
  float phi   = acos(1.0 - 2.0 * rand.x);
  float theta = 2.0 * kPi * rand.y;

  vec3 randOnSphere = getSphericalDir(theta, phi);

  float r = pow(rand.z, 1.0 / 3.0);
  return r * randOnSphere;
}

mat3 makeTBN(vec3 N) {
  vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
  vec3 T  = normalize(cross(up, N));
  vec3 B  = cross(N, T);
  return mat3(T, B, N);
}

vec3 randomCosineWeightedHemispherePoint(vec3 normal, uvec3 seed) {
  vec3 dir = stbnWeightedCosine(seed);
  // change from [0,1] to [-1,1]
  dir = dir * 2.0 - 1.0;

  mat3 TBN = makeTBN(normal);
  return TBN * dir;
}

#endif // RANDOM_GLSL