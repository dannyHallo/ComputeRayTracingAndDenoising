#ifndef HASH_GLSL
#define HASH_GLSL

uint murmurHash(uint src) {
  const uint M = 0x5bd1e995u;
  uint h       = 1190494759u;
  src *= M;
  src ^= src >> 24u;
  src *= M;
  h *= M;
  h ^= src;
  h ^= h >> 13u;
  h *= M;
  h ^= h >> 15u;
  return h;
}

uvec3 murmurHash33(uvec3 src) {
  const uint M = 0x5bd1e995u;
  uvec3 h      = uvec3(1190494759u, 2147483647u, 3559788179u);
  src *= M;
  src ^= src >> 24u;
  src *= M;
  h *= M;
  h ^= src.x;
  h *= M;
  h ^= src.y;
  h *= M;
  h ^= src.z;
  h ^= h >> 13u;
  h *= M;
  h ^= h >> 15u;
  return h;
}

// this hashing function is probably to be the best one of its kind
// https://nullprogram.com/blog/2018/07/31/
uint wellonsHash(uint x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

// 3 outputs, 3 inputs
vec3 hash(vec3 src) {
  uvec3 h = murmurHash33(floatBitsToUint(src));
  return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

#endif // HASH_GLSL
