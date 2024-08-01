#ifndef DDA_MARCHING_GLSL
#define DDA_MARCHING_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/chunking.glsl"

bool _inChunkRange(ivec3 pos) {
  return all(greaterThanEqual(pos, ivec3(0))) && all(lessThan(pos, sceneInfoBuffer.data.chunksDim));
}

bool _hasChunk(uvec3 chunkIndex) {
  return chunkIndicesBuffer
             .data[getChunksBufferLinearIndex(chunkIndex, sceneInfoBuffer.data.chunksDim)] > 0;
}

#define MAX_DDA_ITERATION 50

// this function if used for continuous raymarching, where we need to save the last hit chunk
bool ddaMarchingWithSave(out ivec3 oChunkIndex, inout ivec3 mapPos, inout vec3 sideDist,
                         inout bool enteredBigBoundingBox, inout uint it, vec3 deltaDist,
                         ivec3 rayStep, vec3 o, vec3 d) {
  bvec3 mask;
  while (it++ < MAX_DDA_ITERATION) {
    mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
    sideDist += vec3(mask) * deltaDist;

    oChunkIndex = mapPos;
    mapPos += ivec3(vec3(mask)) * rayStep;

    if (_inChunkRange(oChunkIndex)) {
      enteredBigBoundingBox = true;
      if (_hasChunk(oChunkIndex)) {
        return true;
      }
    }
    // went outside the outer bounding box
    else if (enteredBigBoundingBox) {
      return false;
    }
  }
  return false;
}

#endif // DDA_MARCHING_GLSL
