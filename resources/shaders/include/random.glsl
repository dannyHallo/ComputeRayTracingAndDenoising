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

// rounding off issues:
// https://stackoverflow.com/questions/32231777/how-to-avoid-rounding-off-of-large-float-or-double-values

// shaders:
// this solution avoids float rounding errors
// https://www.shadertoy.com/view/4dtBWH
// https://www.shadertoy.com/view/NdBSWm

const uvec3 kBlueNoiseSize = uvec3(128, 128, 64);
const float invExp         = 1 / exp2(24.);
const int alpha1Large      = 12664746;
const int alpha2Large      = 9560334;
vec2 ldsNoise(uvec3 seed, Disturbance disturb) {
  uint n = hash(seed.x + renderInfoUbo.data.lowResSize.x * seed.y + disturb.d) + seed.z;
  return fract(ivec2(alpha1Large * n, alpha2Large * n) * invExp);
}

vec2 getOffsetFromDisturbance(Disturbance disturb) {
  uint n         = disturb.d + 777123;
  vec2 ldsOffset = fract(ivec2(alpha1Large * n, alpha2Large * n) * invExp);
  return ldsOffset;
}

// returns un-correlated random numbers ranging from [0,1] for each channel
vec2 randomUv(uvec3 seed, Disturbance disturb) {
  vec2 rand;
  const bool useLdsNoise = true;
  if (useLdsNoise) {
    vec2 offsetBasedOnDisturbance = getOffsetFromDisturbance(disturb);

    seed = uvec3(seed.x + offsetBasedOnDisturbance.x * kBlueNoiseSize.x,
                 seed.y + offsetBasedOnDisturbance.y * kBlueNoiseSize.x, seed.z);

    rand = imageLoad(vec2BlueNoise, ivec3(seed.x % kBlueNoiseSize.x, seed.y % kBlueNoiseSize.y,
                                          seed.z % kBlueNoiseSize.z))
               .xy;
  } else {
    rand.x = random(seed);
    rand.y = random(seed);
  }
  return rand;
}

vec3 randomInUnitSphere(uvec3 seed, Disturbance disturb) {
  vec2 rand = randomUv(seed, disturb);

  // (1, -1) => acos => (0, pi), uniform before acos
  float phi   = acos(1.0 - 2.0 * rand.x);
  float theta = 2.0 * kPi * rand.y;

  return getSphericalDir(theta, phi);
}

vec3 randomInHemisphere(vec3 normal, uvec3 seed, Disturbance disturb) {
  vec3 inUnitSphere = randomInUnitSphere(seed, disturb);
  if (dot(inUnitSphere, normal) > 0.0)
    return inUnitSphere;
  else
    return -inUnitSphere;
}

mat3 makeTBN(vec3 N) {
  vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
  vec3 T  = normalize(cross(up, N));
  vec3 B  = cross(N, T);
  return mat3(T, B, N);
}

vec3 randomCosineWeightedHemispherePoint(vec3 normal, uvec3 seed, Disturbance disturb) {
  vec3 dir;
  const bool useLdsNoise = true;
  if (useLdsNoise) {
    vec2 offsetBasedOnDisturbance = getOffsetFromDisturbance(disturb);
    seed                          = uvec3(seed.x + offsetBasedOnDisturbance.x * kBlueNoiseSize.x,
                                          seed.y + offsetBasedOnDisturbance.y * kBlueNoiseSize.x, seed.z);
    dir                           = imageLoad(weightedCosineBlueNoise,
                                              ivec3(seed.x % kBlueNoiseSize.x, seed.y % kBlueNoiseSize.y,
                                                    seed.z % kBlueNoiseSize.z))
              .xyz;
    // change from [0,1] to [-1,1]
    dir = dir * 2.0 - 1.0;

  } else {
    vec2 rand = randomUv(seed, disturb);

    float theta = 2.0 * kPi * rand.x;
    float phi   = acos(sqrt(1.0 - rand.y));

    dir.x = sin(phi) * cos(theta);
    dir.y = sin(phi) * sin(theta);
    dir.z = cos(phi);
  }
  mat3 TBN = makeTBN(normal);
  return TBN * dir;
}

#endif // RANDOM_GLSL