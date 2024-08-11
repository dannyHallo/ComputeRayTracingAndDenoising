#ifndef SEASCAPE_GLSL
#define SEASCAPE_GLSL

/*
 * taken from: https://www.shadertoy.com/view/Ms2SD1
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

const int ITER_GEOMETRY    = 3;
const int ITER_FRAGMENT    = 5;
const float SEA_HEIGHT     = 0.6;
const float SEA_CHOPPY     = 4.0;
const float SEA_SPEED      = 0.8;
const float SEA_FREQ       = 0.16;
const vec3 SEA_BASE        = vec3(0.0, 0.09, 0.18);
const vec3 SEA_WATER_COLOR = vec3(0.8, 0.9, 0.6) * 0.6;
#define SEA_TIME (1.0 + renderInfoUbo.data.time * SEA_SPEED)
const mat2 octave_m = mat2(1.6, 1.2, -1.2, 1.6);

// math
mat3 fromEuler(vec3 ang) {
  vec2 a1 = vec2(sin(ang.x), cos(ang.x));
  vec2 a2 = vec2(sin(ang.y), cos(ang.y));
  vec2 a3 = vec2(sin(ang.z), cos(ang.z));
  mat3 m;
  m[0] = vec3(a1.y * a3.y + a1.x * a2.x * a3.x, a1.y * a2.x * a3.x + a3.y * a1.x, -a2.y * a3.x);
  m[1] = vec3(-a2.y * a1.x, a1.y * a2.y, a2.x);
  m[2] = vec3(a3.y * a1.x * a2.x + a1.y * a3.x, a1.x * a3.x - a1.y * a3.y * a2.x, a2.y * a3.y);
  return m;
}

// the hash function from shadertoy has been repleaced by murmurHash

float noise(in vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  return -1.0 + 2.0 * mix(mix(hash12(i + vec2(0.0, 0.0)), hash12(i + vec2(1.0, 0.0)), u.x),
                          mix(hash12(i + vec2(0.0, 1.0)), hash12(i + vec2(1.0, 1.0)), u.x), u.y);
}

// lighting
float diffuse(vec3 n, vec3 l, float p) { return pow(dot(n, l) * 0.4 + 0.6, p); }
float specular(vec3 n, vec3 l, vec3 e, float s) {
  float nrm = (s + 8.0) / (kPi * 8.0);
  return pow(max(dot(reflect(e, n), l), 0.0), s) * nrm;
}

// sea
float sea_octave(vec2 uv, float choppy) {
  uv += noise(uv);
  //   uv += cnoise(uv);
  vec2 wv  = 1.0 - abs(sin(uv));
  vec2 swv = abs(cos(uv));
  wv       = mix(wv, swv, wv);
  return pow(1.0 - pow(wv.x * wv.y, 0.65), choppy);
}

float map(vec3 p, uint iterationCount) {
  float freq   = SEA_FREQ;
  float amp    = SEA_HEIGHT;
  float choppy = SEA_CHOPPY;
  vec2 uv      = p.xz;
  uv.x *= 0.75;

  float d, h = 0.0;
  for (uint i = 0; i < iterationCount; i++) {
    d = sea_octave((uv + SEA_TIME) * freq, choppy);
    d += sea_octave((uv - SEA_TIME) * freq, choppy);
    h += d * amp;
    uv *= octave_m;
    freq *= 1.9;
    amp *= 0.22;
    choppy = mix(choppy, 1.0, 0.2);
  }
  return p.y - h;
}

vec3 getSeaColor(vec3 p, vec3 n, vec3 eye, float t) {
  vec3 l    = environmentUbo.data.sunDir;
  vec3 dist = p + t * eye;

  float fresnel = clamp(1.0 - dot(n, -eye), 0.0, 1.0);
  fresnel       = min(pow(fresnel, 3.0), 0.5);

  vec3 reflected = skyColor(reflect(eye, n), true);
  vec3 refracted = SEA_BASE + diffuse(n, l, 80.0) * SEA_WATER_COLOR * 0.12;

  vec3 color = mix(refracted, reflected, fresnel);

  float atten = max(1.0 - dot(dist, dist) * 0.001, 0.0);
  color += SEA_WATER_COLOR * (p.y - SEA_HEIGHT) * 0.18 * atten;

  color += vec3(specular(n, l, eye, 60.0));

  return color;
}

vec3 _getNormal(vec3 p, float eps) {
  vec3 n;
  n.y = map(p, ITER_FRAGMENT);
  n.x = map(vec3(p.x + eps, p.y, p.z), ITER_FRAGMENT) - n.y;
  n.z = map(vec3(p.x, p.y, p.z + eps), ITER_FRAGMENT) - n.y;
  n.y = eps;
  return normalize(n);
}

bool _heightMapTracing(out float oT, vec3 o, vec3 d) {
  float tm = 0.0;
  float tx = 1000.0;
  float hx = map(o + d * tx, ITER_GEOMETRY);
  if (hx > 0.0) {
    oT = tx;
    return false;
  }
  float hm   = map(o + d * tm, ITER_GEOMETRY);
  float tmid = 0.0;
  for (int i = 0; i < NUM_STEPS; i++) {
    tmid       = mix(tm, tx, hm / (hm - hx));
    float hmid = map(o + d * tmid, ITER_GEOMETRY);
    if (hmid < 0.0) {
      tx = tmid;
      hx = hmid;
    } else {
      tm = tmid;
      hm = hmid;
    }
  }
  oT = tmid;
  return true;
}

// #define EPSILON_NRM (0.1 / iResolution.x)
#define EPSILON_NRM 1e-4

bool traceSeascape(out vec3 oColor, out vec3 oPosition, out float oT, vec3 o, vec3 d) {
  oColor = vec3(0.0);

  bool hitSea = _heightMapTracing(oT, o, d);
  if (!hitSea) {
    return false;
  }

  oPosition = o + oT * d;
  vec3 n    = _getNormal(oPosition, dot(oT, oT) * EPSILON_NRM);

  oColor = getSeaColor(oPosition, n, d, oT);
  return true;
}

#endif // SEASCAPE_GLSL