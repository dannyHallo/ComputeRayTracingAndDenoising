#ifndef BLOCK_TYPE_GLSL
#define BLOCK_TYPE_GLSL

const uint kBlockTypeEmpty = 0;

const uint kBlockTypeDirt  = 1;
const uint kBlockTypeRock  = 2;
const uint kBlockTypeSand  = 3;

const uint kBlockTypeMax = 3;

// https://colorhunt.co/palette/973131e0a75ef9d689f5e7b2
vec3 getBlockColor(uint blockType) {
  switch (blockType) {
  case kBlockTypeDirt:
    return vec3(151.0, 49.0, 49.0) / 255.0;
  case kBlockTypeRock:
    return vec3(152.0, 125.0, 154.0) / 255.0;
  case kBlockTypeSand:
    return vec3(249.0, 214.0, 137.0) / 255.0;
  default:
    return vec3(0.0);
  }
}

#endif // BLOCK_TYPE_GLSL
