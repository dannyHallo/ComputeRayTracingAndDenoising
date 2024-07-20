#ifndef POST_PROCESSING_GLSL
#define POST_PROCESSING_GLSL

#include "../include/core/color.glsl"

// tunemap operators

// taken from: https://www.shadertoy.com/view/tdSXzD
vec3 jodieReinhardTmo(vec3 c) {
  float l = lum(c);
  vec3 tc = c / (c + 1.0);
  return mix(c / (l + 1.0), tc, tc);
}

vec3 reinhardExtendedLumTmo(vec3 v, float maxWhiteLum) {
  float lOld      = lum(v);
  float numerator = lOld * (1.0f + (lOld / (maxWhiteLum * maxWhiteLum)));
  float lNew      = numerator / (1.0f + lOld);
  return v * (lNew / lOld);
}

// taken from: https://www.shadertoy.com/view/MslGR8
vec3 getDitherMask(ivec2 screenSpaceUv) {
  // bit-depth of display. Normally 8 but some LCD monitors are 7 or even 6-bit.
  float dither_bit = 8.0;
  // calculate grid position
  float grid_position =
      fract(dot(vec2(screenSpaceUv) - vec2(0.5, 0.5), vec2(1.0 / 16.0, 10.0 / 36.0) + 0.25));

  // calculate how big the shift should be
  float dither_shift = (0.25) * (1.0 / (pow(2.0, dither_bit) - 1.0));

  // shift the individual colors differently, thus making it even harder to see
  // the dithering pattern
  vec3 dither_shift_RGB = vec3(dither_shift, -dither_shift, dither_shift); // subpixel dithering

  // modify shift acording to grid position
  dither_shift_RGB = mix(2.0 * dither_shift_RGB, -2.0 * dither_shift_RGB, grid_position);

  // shift the color by dither_shift
  return 0.5 / 255.0 + dither_shift_RGB;
}

#endif // POST_PROCESSING_GLSL