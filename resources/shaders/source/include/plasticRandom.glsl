// guides to low descripancy sequence:
// http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
// https://psychopath.io/post/2014_06_28_low_discrepancy_sequences

// rounding off issues:
// https://stackoverflow.com/questions/32231777/how-to-avoid-rounding-off-of-large-float-or-double-values

// shaders:
// https://www.shadertoy.com/view/4dtBWH
// https://www.shadertoy.com/view/NdBSWm

const float invExp = 1 / exp2(24.);

vec2 ldsNoise(uint n) {
  return fract(vec2(12664746 * n, 9560334 * n) * invExp);
}