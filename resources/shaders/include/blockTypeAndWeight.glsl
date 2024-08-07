#ifndef BLOCK_TYPE_AND_WEIGHT_GLSL
#define BLOCK_TYPE_AND_WEIGHT_GLSL

#include "../include/blockType.glsl"

uint getBlockTypeFromWeight(float weight) {
  if (weight < 0.0) {
    return kBlockTypeEmpty;
  }
  if (weight < 0.01) {
    return kBlockTypeGrass;
  }
  return kBlockTypeDirt;
}

// these are adjustable
const float f           = 1.0 / 100.0;
const float boundaryMin = -f;
const float boundaryMax = f;
const uint nBits        = 12;
const uint nLevels      = (1 << nBits) - 1;
const uint encodedMask  = nLevels;

uint _packWeight(float weight) {
  weight          = clamp(weight, boundaryMin, boundaryMax);
  const float f01 = (weight - boundaryMin) / (boundaryMax - boundaryMin);
  return uint(f01 * nLevels);
}

float _unpackWeight(uint encodedWeight) {
  return (float(encodedWeight) / float(nLevels)) * (boundaryMax - boundaryMin) + boundaryMin;
}

uint packBlockTypeAndWeight(uint blockType, float weight) {
  return (blockType << nBits) | _packWeight(weight);
}

void unpackBlockTypeAndWeight(out uint oBlockType, out float oWeight, uint data) {
  oBlockType = (data >> nBits);
  oWeight    = _unpackWeight(data & encodedMask);
}

#endif // BLOCK_TYPE_AND_WEIGHT_GLSL