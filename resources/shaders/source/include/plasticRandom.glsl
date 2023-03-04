// a comprehensive guide to the r2 low descripancy sequence:
// http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/

// rounding off issues:
// https://stackoverflow.com/questions/32231777/how-to-avoid-rounding-off-of-large-float-or-double-values

// shaders:
// https://www.shadertoy.com/view/4dtBWH
// https://www.shadertoy.com/view/NdBSWm

vec2 plasticRandom(uint index) {
  return fract(vec2(12664746 * index, 9560334 * index) / exp2(24.));
}

const float r10f[10] =
    float[10](0.936069, 0.876225, 0.820208, 0.767771, 0.718687, 0.672740,
              0.629731, 0.589472, 0.551787, 0.516510);

vec2 r2(vec2 seed, float n) {
  vec2 rand;
  rand.x = fract(seed.x + n * r10f[0]);
  rand.y = fract(seed.y + n * r10f[1]);
  return rand;
}

float invExp = 1 / exp2(24.);

vec2 r2_seed(vec2 coord) {
  vec2 seed;

  seed.x = fract((12664746 * coord.x + 9560334 * coord.y) * invExp);
  seed.y = fract((9560334 * coord.x + 12664746 * coord.y) * invExp);

  return seed;
}

vec2 r2_seed(uint n) {
  vec2 seed;

  // seed.x = fract((12664746 * coord.x + 9560334 * coord.y) * invExp);
  // seed.y = fract((9560334 * coord.x + 12664746 * coord.y) * invExp);
  seed.x = fract(12664746 * n * invExp);
  seed.y = fract(9560334 * n * invExp);

  return seed;
}