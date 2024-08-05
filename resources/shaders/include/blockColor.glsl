#ifndef BLOCK_COLOR_GLSL
#define BLOCK_COLOR_GLSL

#include "../include/blockType.glsl"

// return tweakableParametersUbo.data.debugC1;

// https://colorhunt.co/palette/973131e0a75ef9d689f5e7b2
vec3 getBlockColor(uint blockType) {
  switch (blockType) {
  case kBlockTypeDirt:
    return vec3(84.0, 20.0, 20.0) / 255.0;
  case kBlockTypeRock:
    return vec3(128.0, 128.0, 128.0) / 255.0;
  case kBlockTypeSand:
    // return vec3(223.0, 193.0, 158.0) / 255.0;
    return vec3(255.0, 186.0, 239.0) / 255.0;
  default:
    return vec3(0.0);
  }
}

#endif // BLOCK_COLOR_GLSL
