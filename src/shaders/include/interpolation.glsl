#ifndef INTERPOLATION_GLSL
#define INTERPOLATION_GLSL

// Catmull-Rom filtering code from http://vec3.ca/bicubic-filtering-in-fewer-taps/
void bicubicCatmullRom(vec2 uv, out vec2 sampleLocations[3], out vec2 sampleWeights[3]) {
  vec2 tc = floor(uv - 0.5) + 0.5;
  vec2 f  = uv - tc;
  vec2 f2 = f * f;
  vec2 f3 = f2 * f;

  vec2 w0 = f2 - 0.5 * (f3 + f);
  vec2 w1 = 1.5 * f3 - 2.5 * f2 + 1;
  vec2 w3 = 0.5 * (f3 - f2);
  vec2 w2 = 1 - w0 - w1 - w3;

  sampleWeights[0] = w0;
  sampleWeights[1] = w1 + w2;
  sampleWeights[2] = w3;

  sampleLocations[0] = tc - 1;
  sampleLocations[1] = tc + w2 / sampleWeights[1];
  sampleLocations[2] = tc + 2;
}

#endif // INTERPOLATION_GLSL