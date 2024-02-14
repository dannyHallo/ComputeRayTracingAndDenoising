#ifndef COLOR_PACKER_H
#define COLOR_PACKER_H

uint packRGBE(vec3 v) {
  vec3 va       = max(vec3(0), v);
  float max_abs = max(va.r, max(va.g, va.b));
  if (max_abs == 0) return 0;

  float exponent = floor(log2(max_abs));

  uint result;
  result = uint(clamp(exponent + 20, 0, 31)) << 27;

  float scale = pow(2, -exponent) * 256.0;
  uvec3 vu    = min(uvec3(511), uvec3(round(va * scale)));
  result |= vu.r;
  result |= vu.g << 9;
  result |= vu.b << 18;

  return result;
}

vec3 unpackRGBE(uint x) {
  int exponent = int(x >> 27) - 20;
  float scale  = pow(2, exponent) / 256.0;

  vec3 v;
  v.r = float(x & 0x1ff) * scale;
  v.g = float((x >> 9) & 0x1ff) * scale;
  v.b = float((x >> 18) & 0x1ff) * scale;

  return v;
}

#endif // COLOR_PACKER_H