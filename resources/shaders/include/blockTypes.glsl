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

const float f = 1.0 / 100.0;
const float boundaryMin = -f;
const float boundaryMax = f;

uint packWeight(float weight) {
  weight = clamp(weight, boundaryMin, boundaryMax);
  return uint((weight - boundaryMin) / (boundaryMax - boundaryMin) * 255.0);
}

float unpackWeight(uint encodedWeight) {
  return (float(encodedWeight) / 255.0) * (boundaryMax - boundaryMin) + boundaryMin;
}

uint packBlockTypeAndWeight(uint blockType, float weight) {
  return (blockType << 8) | packWeight(weight);
}

void unpackBlockTypeAndWeight(out uint oBlockType, out float oWeight, uint data) {
  oBlockType = (data >> 8) & 0xFF;
  oWeight    = unpackWeight(data & 0xFF);
}

#endif // BLOCK_TYPES_GLSL
