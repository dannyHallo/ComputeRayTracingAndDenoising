// currently, every file that includes this file should have globalUbo in their
// layout
const float pi = 3.1415926535897932385;

uint rngState = 0;

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
  const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

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

// structure of uvec3 seed:
// (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, globalUbo.currentSample)

float random(uvec3 seed) {
  if (rngState == 0) {
    uint index = seed.x + globalUbo.swapchainWidth * seed.y + 1;
    rngState   = index * globalUbo.currentSample + 1;
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

vec2 ldsNoise2d(uint x, uint y) {
  uint n     = x + y * 57u;
  n          = (n << 13u) ^ n;
  uint a     = 2750769u;
  uint b     = 60493u;
  uint h     = 16u;
  uint m     = 4294967291u;
  uint k     = n * a;
  uint l     = n * b;
  uint p     = k + l;
  p          = (p >> h) ^ p;
  p          = (p >> h) ^ p;
  vec2 noise = vec2(float(p % m) / float(m), float((p * a) % m) / float(m));
  return noise;
}

// const float alpha1 = 0.7548776662466927;
// const float alpha2 = 0.5698402909980532;
const float invExp    = 1 / exp2(24.);
const int alpha1Large = 12664746;
const int alpha2Large = 9560334;

vec2 ldsNoise(uint n) {
  return fract(vec2(alpha1Large * n, alpha2Large * n) * invExp);
}

vec2 ldsNoiseR2(uvec3 seed) {
  // Calculate the total number of pixels
  uint totalPixels = globalUbo.swapchainWidth * globalUbo.swapchainHeight;

  // Calculate a unique index for the current pixel and sample number
  uint pixelIndex = seed.y * globalUbo.swapchainWidth + seed.x;

  uint globalSampleNum = totalPixels * seed.z + pixelIndex;

  return ldsNoise(globalSampleNum);
}

// Returns a random real in [min,max).
// float random(float min, float max) { return min + (max - min) * random(); }

vec2 randomUv(uvec3 seed, bool useLdsNoise) {
  // vec2 rand = ldsNoise2d(seed.x, seed.y);
  vec2 rand;
  if (useLdsNoise) {
    rand = ldsNoiseR2(seed);
  } else {
    rand.x = random(seed);
    rand.y = random(seed);
  }
  return rand;
}

// ---- Low discrepancy noise
vec3 randomInUnitSphere(uvec3 seed, bool useLdsNoise) {
  vec2 rand = randomUv(seed, useLdsNoise);

  float phi   = acos(1 - 2 * rand.x);
  float theta = 2 * pi * rand.y;

  float x = sin(phi) * cos(theta);
  float y = sin(phi) * sin(theta);
  float z = cos(phi);

  return vec3(x, y, z);
}

// its pdf is 1 / (2 * pi)
vec3 randomInHemisphere(vec3 normal, uvec3 seed, bool useLdsNoise) {
  vec3 inUnitSphere = randomInUnitSphere(seed, useLdsNoise);
  if (dot(inUnitSphere, normal) > 0.0)
    return inUnitSphere;
  else
    return -inUnitSphere;
}

// its pdf is 1 / pi
vec3 randomCosineWeightedHemispherePoint(vec3 normal, uvec3 seed,
                                         bool useLdsNoise) {
  vec2 rand = randomUv(seed, useLdsNoise);

  float theta = 2.0 * pi * rand.x;
  float phi   = acos(sqrt(1.0 - rand.y));

  vec3 dir;
  dir.x = sin(phi) * cos(theta);
  dir.y = sin(phi) * sin(theta);
  dir.z = cos(phi);

  // Create an orthonormal basis to transform the direction
  vec3 tangent, bitangent;
  if (abs(normal.x) > abs(normal.y))
    tangent = normalize(cross(vec3(0.0, 1.0, 0.0), normal));
  else
    tangent = normalize(cross(vec3(1.0, 0.0, 0.0), normal));
  bitangent = cross(normal, tangent);

  // Transform the direction from the hemisphere's local coordinates to world
  // coordinates
  vec3 samp = dir.x * tangent + dir.y * bitangent + dir.z * normal;

  return samp;
}
