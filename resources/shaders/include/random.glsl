#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/core/definitions.glsl"
#include "../include/core/geom.glsl"

// for easier searching
struct Disturbance {
  uint d;
};

// this hashing function is probably to be the best one of its kind
// https://nullprogram.com/blog/2018/07/31/
uint hash(uint x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value
// below 1.0.
float floatConstruct(uint m) {
  const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
  const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

  m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
  m |= ieeeOne;      // Add fractional part to 1.0

  float f = uintBitsToFloat(m); // Range [1:2]
  return f - 1.0;               // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random(uint x) { return floatConstruct(hash(x)); }

uint rngState = 0;
float random(uvec3 seed) {
  if (rngState == 0) {
    uint index = seed.x + renderInfoUbo.data.lowResSize.x * seed.y + 1;
    rngState   = index * renderInfoUbo.data.currentSample + 1;
  } else {
    rngState = hash(rngState);
  }
  return random(rngState);
}

// guides to low descripancy sequence:
// http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences
// https://psychopath.io/post/2014_06_28_low_discrepancy_sequences
// https://github.com/NVIDIAGameWorks/SpatiotemporalBlueNoiseSDK?tab=readme-ov-file

// rounding off issues:
// https://stackoverflow.com/questions/32231777/how-to-avoid-rounding-off-of-large-float-or-double-values

// shaders:
// this solution avoids float rounding errors
// https://www.shadertoy.com/view/4dtBWH
// https://www.shadertoy.com/view/NdBSWm

const uvec3 kBlueNoiseSize = uvec3(128, 128, 64);
const float invExp         = 1 / exp2(24.);

vec2 getOffsetFromDisturbance(Disturbance disturb) {
  uint n         = disturb.d + 777123;
  vec2 ldsOffset = fract(ivec2(12664746 * n, 9560334 * n) * invExp);
  return ldsOffset;
}

// returns un-correlated random numbers ranging from [0,1] for each channel
vec2 randomUv(uvec3 seed, Disturbance disturb) {
  vec2 offsetBasedOnDisturbance = getOffsetFromDisturbance(disturb);
  seed                          = uvec3(seed.x + offsetBasedOnDisturbance.x * kBlueNoiseSize.x,
                                        seed.y + offsetBasedOnDisturbance.y * kBlueNoiseSize.x, seed.z);
  return imageLoad(vec2BlueNoise, ivec3(seed.x % kBlueNoiseSize.x, seed.y % kBlueNoiseSize.y,
                                        seed.z % kBlueNoiseSize.z))
      .rg;
}

vec3 randomPointOnSphere(uvec3 seed, Disturbance disturb) {
  vec2 rand = randomUv(seed, disturb);

  // (1, -1) => acos => (0, pi), uniform before acos
  float phi   = acos(1.0 - 2.0 * rand.x);
  float theta = 2.0 * kPi * rand.y;

  return getSphericalDir(theta, phi);
}

vec3 randomPointOnHemisphere(vec3 normal, uvec3 seed, Disturbance disturb) {
  vec3 pointOnSphere = randomPointOnSphere(seed, disturb);
  if (dot(pointOnSphere, normal) > 0.0)
    return pointOnSphere;
  else
    return -pointOnSphere;
}

vec3 randomPointInSphere(uvec3 seed, Disturbance disturb) {
  vec2 offsetBasedOnDisturbance = getOffsetFromDisturbance(disturb);
  seed                          = uvec3(seed.x + offsetBasedOnDisturbance.x * kBlueNoiseSize.x,
                                        seed.y + offsetBasedOnDisturbance.y * kBlueNoiseSize.x, seed.z);
  vec3 rand = imageLoad(vec3BlueNoise, ivec3(seed.x % kBlueNoiseSize.x, seed.y % kBlueNoiseSize.y,
                                             seed.z % kBlueNoiseSize.z))
                  .rgb;

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

vec3 randomCosineWeightedHemispherePoint(vec3 normal, uvec3 seed, Disturbance disturb) {
  vec2 offsetBasedOnDisturbance = getOffsetFromDisturbance(disturb);
  seed                          = uvec3(seed.x + offsetBasedOnDisturbance.x * kBlueNoiseSize.x,
                                        seed.y + offsetBasedOnDisturbance.y * kBlueNoiseSize.x, seed.z);
  vec3 dir =
      imageLoad(weightedCosineBlueNoise, ivec3(seed.x % kBlueNoiseSize.x, seed.y % kBlueNoiseSize.y,
                                               seed.z % kBlueNoiseSize.z))
          .rgb;
  // change from [0,1] to [-1,1]
  dir = dir * 2.0 - 1.0;

  mat3 TBN = makeTBN(normal);
  return TBN * dir;
}

#endif // RANDOM_GLSL