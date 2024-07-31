#ifndef SIMPLEX_NOISE_GLSL
#define SIMPLEX_NOISE_GLSL

//	--------------------------------------------------------------------
//	Optimized implementation of simplex noise.
//	Based on stegu's simplex noise: https://github.com/stegu/webgl-noise.
//	Contact : atyuwen@gmail.com
//	Author : Yuwen Wu (https://atyuwen.github.io/)
//	License : Distributed under the MIT License.
//	--------------------------------------------------------------------

// Permuted congruential generator (only top 16 bits are well shuffled).
// References: 1. Mark Jarzynski and Marc Olano, "Hash Functions for GPU Rendering".
//             2. UnrealEngine/Random.ush. https://github.com/EpicGames/UnrealEngine

// taken from: https://github.com/atyuwen/bitangent_noise?tab=readme-ov-file
// translated from HLSL

uint pcg3d16(uvec3 p) {
  uvec3 v = p * 1664525u + 1013904223u;
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  v.x += v.y * v.z;
  return v.x;
}

uint pcg4d16(uvec4 p) {
  uvec4 v = p * 1664525u + 1013904223u;
  v.x += v.y * v.w;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  v.w += v.y * v.z;
  v.x += v.y * v.w;
  return v.x;
}

// Get random gradient from hash value.
vec3 gradient3d(uint hash) {
  vec3 g = vec3(hash.xxx & uvec3(0x80000, 0x40000, 0x20000));
  return g * vec3(1.0 / 0x40000, 1.0 / 0x20000, 1.0 / 0x10000) - 1.0;
}
vec4 gradient4d(uint hash) {
  vec4 g = vec4(hash.xxxx & uvec4(0x80000, 0x40000, 0x20000, 0x10000));
  return g * vec4(1.0 / 0x40000, 1.0 / 0x20000, 1.0 / 0x10000, 1.0 / 0x8000) - 1.0;
}

// 3D Simplex Noise. Approximately 71 instruction slots used.
// Assume p is in the range [-32768, 32767].
float snoise(vec3 p) {
  const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
  const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

  // First corner
  vec3 i  = floor(p + dot(p, C.yyy));
  vec3 x0 = p - i + dot(i, C.xxx);

  // Other corners
  vec3 g  = step(x0.yzx, x0.xyz);
  vec3 l  = 1.0 - g;
  vec3 i1 = min(g.xyz, l.zxy);
  vec3 i2 = max(g.xyz, l.zxy);

  // x0 = x0 - 0.0 + 0.0 * C.xxx;
  // x1 = x0 - i1  + 1.0 * C.xxx;
  // x2 = x0 - i2  + 2.0 * C.xxx;
  // x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

  i          = i + 32768.5;
  uint hash0 = pcg3d16(uvec3(i));
  uint hash1 = pcg3d16(uvec3(i + i1));
  uint hash2 = pcg3d16(uvec3(i + i2));
  uint hash3 = pcg3d16(uvec3(i + 1));

  vec3 p0 = gradient3d(hash0);
  vec3 p1 = gradient3d(hash1);
  vec3 p2 = gradient3d(hash2);
  vec3 p3 = gradient3d(hash3);

  // Mix final noise value.
  vec4 m  = clamp(0.5 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0, 1.0);
  vec4 mt = m * m;
  vec4 m4 = mt * mt;
  return 62.6 * dot(m4, vec4(dot(x0, p0), dot(x1, p1), dot(x2, p2), dot(x3, p3)));
}

// 4D Simplex Noise. Approximately 113 instruction slots used.
// Assume p is in the range [-32768, 32767].
float snoise(vec4 p) {
  const vec4 F4 = vec4(0.309016994374947451);
  const vec4 C  = vec4(0.138196601125011,   // (5 - sqrt(5))/20  G4
                       0.276393202250021,   // 2 * G4
                       0.414589803375032,   // 3 * G4
                       -0.447213595499958); // -1 + 4 * G4

  // First corner
  vec4 i  = floor(p + dot(p, F4));
  vec4 x0 = p - i + dot(i, C.xxxx);

  // Other corners

  // Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
  vec4 i0;
  vec3 isX  = step(x0.yzw, x0.xxx);
  vec3 isYZ = step(x0.zww, x0.yyz);
  // i0.x = dot( isX, vec3( 1.0 ) );
  i0.x   = isX.x + isX.y + isX.z;
  i0.yzw = 1.0 - isX;
  // i0.y += dot( isYZ.xy, vec2( 1.0 ) );
  i0.y += isYZ.x + isYZ.y;
  i0.zw += 1.0 - isYZ.xy;
  i0.z += isYZ.z;
  i0.w += 1.0 - isYZ.z;

  // i0 now contains the unique values 0,1,2,3 in each channel
  vec4 i3 = clamp(i0, 0.0, 1.0);
  vec4 i2 = clamp(i0 - 1.0, 0.0, 1.0);
  vec4 i1 = clamp(i0 - 2.0, 0.0, 1.0);

  // x0 = x0 - 0.0 + 0.0 * C.xxxx
  // x1 = x0 - i1  + 1.0 * C.xxxx
  // x2 = x0 - i2  + 2.0 * C.xxxx
  // x3 = x0 - i3  + 3.0 * C.xxxx
  // x4 = x0 - 1.0 + 4.0 * C.xxxx
  vec4 x1 = x0 - i1 + C.xxxx;
  vec4 x2 = x0 - i2 + C.yyyy;
  vec4 x3 = x0 - i3 + C.zzzz;
  vec4 x4 = x0 + C.wwww;

  i          = i + 32768.5;
  uint hash0 = pcg4d16(uvec4(i));
  uint hash1 = pcg4d16(uvec4(i + i1));
  uint hash2 = pcg4d16(uvec4(i + i2));
  uint hash3 = pcg4d16(uvec4(i + i3));
  uint hash4 = pcg4d16(uvec4(i + 1));

  vec4 p0 = gradient4d(hash0);
  vec4 p1 = gradient4d(hash1);
  vec4 p2 = gradient4d(hash2);
  vec4 p3 = gradient4d(hash3);
  vec4 p4 = gradient4d(hash4);

  // Mix contributions from the five corners
  vec3 m0  = clamp(0.6 - vec3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0, 1.0);
  vec2 m1  = clamp(0.6 - vec2(dot(x3, x3), dot(x4, x4)), 0.0, 1.0);
  vec3 m03 = m0 * m0 * m0;
  vec2 m13 = m1 * m1 * m1;
  return (dot(m03, vec3(dot(p0, x0), dot(p1, x1), dot(p2, x2))) +
          dot(m13, vec2(dot(p3, x3), dot(p4, x4)))) *
         9.0;
}

#endif // SIMPLEX_NOISE_GLSL
