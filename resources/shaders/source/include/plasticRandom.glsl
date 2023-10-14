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

vec2 ldsNoiseR2(uint x, uint y, uint sampleNum) {
  // Calculate the total number of pixels
  uint totalPixels = globalUbo.swapchainWidth * globalUbo.swapchainHeight;

  // Calculate a unique index for the current pixel and sample number
  uint pixelIndex      = y * globalUbo.swapchainWidth + x;
  uint globalSampleNum = totalPixels * sampleNum + pixelIndex;

  return ldsNoise(globalSampleNum);
}