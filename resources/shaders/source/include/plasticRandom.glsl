// guides to low descripancy sequence:
// http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences
// https://psychopath.io/post/2014_06_28_low_discrepancy_sequences

// rounding off issues:
// https://stackoverflow.com/questions/32231777/how-to-avoid-rounding-off-of-large-float-or-double-values

// shaders:
// https://www.shadertoy.com/view/4dtBWH
// https://www.shadertoy.com/view/NdBSWm

const float invExp = 1 / exp2(24.);

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

const alpha1 = 0.7548776662466927;
const alpha2 = 0.5698402909980532;

vec2 ldsNoiseR2(uint x, uint y, uint sampleNum) {
    uint resolution = 1u << 10u; // E.g. assume a 1024x1024 texture
    uint pixelIndex = y * resolution + x;
    uint globalSampleNum = pixelIndex + sampleNum;

    // Convert to float for computation
    float globalSampleNumFloat = float(globalSampleNum);

    return vec2(
        fract(globalSampleNumFloat * alpha1),
        fract(globalSampleNumFloat * alpha2)
    );
}