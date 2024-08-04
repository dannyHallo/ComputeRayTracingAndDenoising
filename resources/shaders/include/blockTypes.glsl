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

// these are adjustable
const float f           = 1.0 / 100.0;
const float boundaryMin = -f;
const float boundaryMax = f;
const uint nBits        = 12;
const uint nLevels      = (1 << nBits) - 1;
const uint encodedMask  = nLevels;

uint packWeight(float weight) {
  weight          = clamp(weight, boundaryMin, boundaryMax);
  const float f01 = (weight - boundaryMin) / (boundaryMax - boundaryMin);
  return uint(f01 * nLevels);
}

float unpackWeight(uint encodedWeight) {
  return (float(encodedWeight) / float(nLevels)) * (boundaryMax - boundaryMin) + boundaryMin;
}

uint packBlockTypeAndWeight(uint blockType, float weight) {
  return (blockType << nBits) | packWeight(weight);
}

void unpackBlockTypeAndWeight(out uint oBlockType, out float oWeight, uint data) {
  oBlockType = (data >> nBits);
  oWeight    = unpackWeight(data & encodedMask);
}

#endif // BLOCK_TYPES_GLSL
