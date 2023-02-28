#include "plasticRandom.glsl"

const float pi = 3.1415926535897932385;

uint index =
    ubo.currentSample + gl_GlobalInvocationID.x + imageSize(rawTex).x * gl_GlobalInvocationID.y + 1;

uint rngState = index * ubo.currentSample + 1;

// ---- Random
// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash(uint x) {
  x += (x << 10u);
  x ^= (x >> 6u);
  x += (x << 3u);
  x ^= (x >> 11u);
  x += (x << 15u);
  return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash(uvec2 v) { return hash(v.x ^ hash(v.y)); }
uint hash(uvec3 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z)); }
uint hash(uvec4 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w)); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value
// below 1.0.
float floatConstruct(uint m) {
  const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
  const uint ieeeOne = 0x3F800000u;      // 1.0 in IEEE binary32

  m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
  m |= ieeeOne;      // Add fractional part to 1.0

  float f = uintBitsToFloat(m); // Range [1:2]
  return f - 1.0;               // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random(uint x) { return floatConstruct(hash(x)); }
float random(float x) { return floatConstruct(hash(floatBitsToUint(x))); }
float random(vec2 v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random(vec3 v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random(vec4 v) { return floatConstruct(hash(floatBitsToUint(v))); }

float random() {
  rngState = hash(rngState);
  return random(rngState);
}

// Returns a random real in [min,max).
float random(float min, float max) { return min + (max - min) * random(); }

// ---- Low discrepancy noise
vec3 random_in_unit_sphere() {
  float seed2[2];
  float rand2[2];

  // r2_seed(gl_GlobalInvocationID.xy, seed2);
  // r2(seed2, ubo.currentSample, rand2);

  for(int i = 0; i < 2; i++)
    rand2[i] = random();

  float phi = acos(1 - 2 * rand2[0]);
  float theta = 2 * pi * rand2[1];

  float x = sin(phi) * cos(theta);
  float y = sin(phi) * sin(theta);
  float z = cos(phi);

  return vec3(x, y, z);
}

vec3 random_in_hemisphere(vec3 normal) {
  vec3 in_unit_sphere = random_in_unit_sphere();
  if (dot(in_unit_sphere, normal) > 0.0)
    return in_unit_sphere;
  else
    return -in_unit_sphere;
}

vec3 random_cosine_direction() {
  float r1 = random();
  float r2 = random();
  float z = sqrt(1 - r2);

  float phi = 2 * pi * r1;
  float x = cos(phi) * sqrt(r2);
  float y = sin(phi) * sqrt(r2);

  return vec3(x, y, z);
}
