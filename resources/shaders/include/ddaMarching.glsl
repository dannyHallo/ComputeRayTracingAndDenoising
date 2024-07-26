#ifndef DDA_MARCHING_GLSL
#define DDA_MARCHING_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/chunksBufferLayout.glsl"

bool _inChunkRange(ivec3 pos) {
  return all(greaterThanEqual(pos, ivec3(0))) && all(lessThan(pos, sceneInfoBuffer.data.chunksDim));
}

bool _hasChunk(uvec3 chunkIndex) {
  return chunksBuffer.data[getChunksBufferLinearIndex(chunkIndex, sceneInfoBuffer.data.chunksDim)] >
         0;
}

#define MAX_DDA_ITERATION 50

// classical branchless DDA
// this function is deprecated and is kept for reference
// the algorithm is originally from https://www.shadertoy.com/view/4dX3zl
bool ddaMarching(out ivec3 oHitChunkPosOffset, out uvec3 oHitChunkLookupOffset, vec3 o, vec3 d) {
  ivec3 mapPos   = ivec3(floor(o));
  vec3 deltaDist = 1.0 / abs(d);
  ivec3 rayStep  = ivec3(sign(d));
  vec3 sideDist  = (((sign(d) * 0.5) + 0.5) + sign(d) * (vec3(mapPos) - o)) * deltaDist;

  bvec3 mask;
  bool enteredBigBoundingBox = false;
  for (int i = 0; i < MAX_DDA_ITERATION; i++) {
    ivec3 lookupPos = mapPos + ivec3((sceneInfoBuffer.data.chunksDim - 1) / 2);

    if (_inChunkRange(lookupPos)) {
      enteredBigBoundingBox = true;
      if (_hasChunk(lookupPos)) {
        oHitChunkPosOffset    = mapPos;
        oHitChunkLookupOffset = lookupPos;
        return true;
      }
    }
    // used to be inside the outer chunk range, but now outside, no need to check further
    else if (enteredBigBoundingBox) {
      return false;
    }

    mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
    sideDist += vec3(mask) * deltaDist;
    mapPos += ivec3(vec3(mask)) * rayStep;
  }
  return false;
}

// this function if used for continuous raymarching, where we need to save the last hit chunk
bool ddaMarchingWithSave(out ivec3 oHitChunkPosOffset, out uvec3 oHitChunkLookupOffset,
                         inout ivec3 mapPos, inout vec3 sideDist, inout bool enteredBigBoundingBox,
                         inout uint it, vec3 deltaDist, ivec3 rayStep, vec3 o, vec3 d) {
  bvec3 mask;
  while (it++ < MAX_DDA_ITERATION) {
    ivec3 lookupPos = mapPos + ivec3((sceneInfoBuffer.data.chunksDim - 1) / 2);

    mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
    sideDist += vec3(mask) * deltaDist;

    ivec3 thisMapPos = mapPos;
    mapPos += ivec3(vec3(mask)) * rayStep;

    if (_inChunkRange(lookupPos)) {
      enteredBigBoundingBox = true;
      if (_hasChunk(lookupPos)) {
        oHitChunkPosOffset    = thisMapPos;
        oHitChunkLookupOffset = lookupPos;
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
