const float pi      = 3.1415926535897932385;
const float epsilon = 1e-8;

const float weightsATrous3x3[3] = float[](.17, .66, .17);
const float weightsATrous5x5[5] = float[](.0625, .25, .375, .25, .0625);

// const int kernelSize = 5;
// int kernalHalfSize   = (kernelSize - 1) / 2;

float rgbToLuminance(vec4 rgba) { return rgba.x * 0.2989 + rgba.y * 0.5870 + rgba.z * 0.1140; }