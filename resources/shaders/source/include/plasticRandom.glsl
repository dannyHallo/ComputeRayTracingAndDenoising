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

void r2(float seed[2], float n, out float dest[2]) {
  for (int i = 0; i < 2; i++)
    dest[i] = fract(seed[i] + n * r10f[i]);
}

void r2_seed(vec2 coord, out float dest[2]) {
  // init the sequence using screen coordinates
  float rand[2];
  rand[0] = (12664746 * coord.x + 9560334 * coord.y) / exp2(24.);
  rand[1] = (9560334 * coord.x + 12664746 * coord.y) / exp2(24.);

  for (int i = 0; i < 2; i++) {
    // advance the 3rd dimention (i)
    float xf = fract(rand[i]);

    // make triangular
    dest[i] = (xf < 0.5) ? (xf * 2.0) : (2.0 - xf * 2.0);
  }
}