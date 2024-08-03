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

#endif // BLOCK_TYPES_GLSL