
#ifndef SEASCAPE_GLSL
#define SEASCAPE_GLSL

/*
 * taken from: https://www.shadertoy.com/view/Ms2SD1 for the _noise functions
 * taken from: https://www.shadertoy.com/view/MdXyzX for the marching functions
 * "Seascape" by Alexander Alekseev aka TDM - 2014
 * License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
 * Contact: tdmaav@gmail.com
 */

#include "../include/svoTracerDescriptorSetLayouts.glsl"

// #include "../include/core/cnoise.glsl"
#include "../include/core/definitions.glsl"
#include "../include/core/hash.glsl"
#include "../include/skyColor.glsl"

const int NUM_STEPS = 8;
const float EPSILON = 1e-3;

const float kWaterTopHeight    = 0.05;
const float kWaterBottomHeight = 0.01;

const int ITER_RAYMARCH = 3;
const int ITER_NORMAL   = 5;
const float SEA_CHOPPY  = 4.0;
const float SEA_FREQ    = 0.6;
#define SEA_TIME (1.0 + renderInfoUbo.data.time * 0.5)
const mat2 octave_m = mat2(1.6, 1.2, -1.2, 1.6);

// -1.0 - 1.0
float _noise(in vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  return 2.0 * mix(mix(hash12(i + vec2(0.0, 0.0)), hash12(i + vec2(1.0, 0.0)), u.x),
                   mix(hash12(i + vec2(0.0, 1.0)), hash12(i + vec2(1.0, 1.0)), u.x), u.y) -
         1.0;
}

float _seaOctave(vec2 uv, float choppy) {
  uv += _noise(uv);
  vec2 wv  = 1.0 - abs(sin(uv));
  vec2 swv = abs(cos(uv));
  wv       = mix(wv, swv, wv);
  return pow(1.0 - pow(wv.x * wv.y, 0.65), choppy);
}

// 0.0 - 1.0
float _getWaveHeight01(vec2 p, uint iterationCount) {
  float freq   = SEA_FREQ;
  float amp    = 1.0;
  float choppy = SEA_CHOPPY;
  vec2 uv      = p;
  uv.x *= 0.75;

  float d, h = 0.0;
  float sumOfAmp = 0.0;
  for (uint i = 0; i < iterationCount; i++) {
    // 0.0 - 1.0
    d = _seaOctave((uv + SEA_TIME) * freq, choppy);
    d += _seaOctave((uv - SEA_TIME) * freq, choppy);
    d /= 2.0;
    sumOfAmp += amp;
    h += d * amp;
    uv *= octave_m;
    freq *= 1.9;
    amp *= 0.22;
    choppy = mix(choppy, 1.0, 0.2);
  }
  h /= sumOfAmp;
  return h;
}

// ray-plane intersection checker
float _intersectPlane(vec3 origin, vec3 direction, vec3 point, vec3 normal) {
  return clamp(dot(point - origin, normal) / dot(direction, normal), -1.0, 9991999.0);
}

// assertion: water must be hit before calling this function
// raymarches the ray from top water layer boundary to low water layer boundary
float _raymarchWater(vec3 o, vec3 d) {
  // calculate intersections and reconstruct positions
  float tHighPlane = _intersectPlane(o, d, vec3(0.0, kWaterTopHeight, 0.0), vec3(0.0, 1.0, 0.0));
  float tLowPlane  = _intersectPlane(o, d, vec3(0.0, kWaterBottomHeight, 0.0), vec3(0.0, 1.0, 0.0));
  vec3 start       = o + tHighPlane * d; // high hit position
  vec3 end         = o + tLowPlane * d;  // low hit position

  float t  = tHighPlane;
  vec3 pos = start;

  for (int i = 0; i < 64; i++) {
    float h = mix(kWaterBottomHeight, kWaterTopHeight, _getWaveHeight01(pos.xz, ITER_RAYMARCH));

    if (h + 1e-3 * t > pos.y) {
      return t;
    }
    t += pos.y - h;
    // iterate forwards according to the height mismatch
    pos = o + t * d;
  }
  // if hit was not registered, just assume hit the top layer,
  // this makes the raymarching faster and looks better at higher distances
  return tHighPlane;
}

// calculate normal at point by calculating the height at the pos and 2 additional points very
// close to pos
vec3 normalCalc(vec2 pos, float eps) {
  vec2 ex     = vec2(eps, 0);
  float depth = kWaterTopHeight - kWaterBottomHeight;
  float h     = _getWaveHeight01(pos, ITER_NORMAL) * depth;
  vec3 a      = vec3(pos.x, h, pos.y);
  return normalize(
      cross(a - vec3(pos.x - eps, _getWaveHeight01(pos - ex.xy, ITER_NORMAL) * depth, pos.y),
            a - vec3(pos.x, _getWaveHeight01(pos + ex.yx, ITER_NORMAL) * depth, pos.y + eps)));
}

bool traceSeascape(out vec3 oPosition, out vec3 oNormal, out float oT, vec3 o, vec3 d) {
  oT        = 1e10;
  oPosition = o + d * oT;

  if (d.y >= 0.0) {
    return false;
  }

  // raymatch water and reconstruct the hit pos
  float dist       = _raymarchWater(o, d);
  vec3 waterHitPos = o + d * dist;

  vec3 N = normalCalc(waterHitPos.xz, 0.01);

  // float fresnel = (0.04 + (1.0 - 0.04) * (pow(1.0 - max(0.0, dot(-N, d)), 5.0)));

  oT        = dist;
  oPosition = waterHitPos;

  // smooth the normal with distance to avoid disturbing high frequency _noise
  oNormal = mix(N, vec3(0.0, 1.0, 0.0), 0.8 * min(1.0, sqrt(dist * 0.01) * 1.1));

  return true;
}

#endif // SEASCAPE_GLSL
