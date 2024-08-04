#ifndef BLOCK_TYPES_GLSL
#define BLOCK_TYPES_GLSL

const uint kBlockTypeEmpty = 0;
const uint kBlockTypeDirt  = 1;
const uint kBlockTypeRock  = 2;

uint getBlockTypeFromWeight(float weight) {
  if (weight < 0.0) {
    return kBlockTypeEmpty;
  }
  if (weight < 0.1) {
    return kBlockTypeDirt;
  }
  return kBlockTypeRock;
}

const float boundaryMin = -0.01;
const float boundaryMax = 0.01;

uint packWeight(float weight) {
  weight = clamp(weight, boundaryMin, boundaryMax);
  return uint((weight - boundaryMin) / (boundaryMax - boundaryMin) * 255.0);
}

float unpackWeight(uint encodedWeight) {
  return (float(encodedWeight) / 255.0) * (boundaryMax - boundaryMin) + boundaryMin;
}

#endif // BLOCK_TYPES_GLSL