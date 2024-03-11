bool inChunkRange(ivec3 pos) {
  return all(greaterThanEqual(pos, ivec3(0))) && all(lessThan(pos, sceneInfoBuffer.data.chunksDim));
}

bool hasChunk(ivec3 pos) { return imageLoad(chunksImage, pos).x > 0; }

#define MAX_DDA_ITERATION 100
#define SMART_MARCHING

#ifdef SMART_MARCHING
// branchless DDA
// originally from https://www.shadertoy.com/view/4dX3zl
// my fork: https://www.shadertoy.com/view/l3fXWH
bool ddaMarching(out ivec3 oHitChunkPosOffset, out uvec3 oHitChunkLookupOffset, uint skippingNum,
                 vec3 o, vec3 d) {
  oHitChunkPosOffset    = ivec3(0);
  oHitChunkLookupOffset = uvec3(0);

  ivec3 mapPos   = ivec3(floor(o));
  vec3 deltaDist = 1.0 / abs(d);
  ivec3 rayStep  = ivec3(sign(d));
  vec3 sideDist  = (((sign(d) * 0.5) + 0.5) + sign(d) * (vec3(mapPos) - o)) * deltaDist;

  bvec3 mask;
  bool enteredBigBoundingBox = false;
  for (int i = 0; i <= MAX_DDA_ITERATION; i++) {
    ivec3 lookupPos = mapPos + ivec3((sceneInfoBuffer.data.chunksDim - 1) / 2);

    if (inChunkRange(lookupPos)) {
      enteredBigBoundingBox = true;
      if (i >= skippingNum && hasChunk(lookupPos)) {
        oHitChunkPosOffset    = mapPos;
        oHitChunkLookupOffset = lookupPos;
        return true;
      }
    }
    // used to be inside the outer chunk range, but now outside, no need to check further
    else {
      if (enteredBigBoundingBox) {
        return false;
      }
    }

    if (i == MAX_DDA_ITERATION) {
      mask = bvec3(false);
      return false;
    }

    // hereby "branchless"
    mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
    sideDist += vec3(mask) * deltaDist;
    mapPos += ivec3(vec3(mask)) * rayStep;
  }
}
#else
bool ddaMarching(out ivec3 oHitChunkPosOffset, out uvec3 oHitChunkLookupOffset, uint skippingNum,
                 vec3 o, vec3 d) {
  oHitChunkPosOffset    = ivec3(0);
  oHitChunkLookupOffset = uvec3(0);

  ivec3 mapPos   = ivec3(floor(o));
  vec3 deltaDist = 1.0 / abs(d);
  ivec3 rayStep  = ivec3(sign(d));
  vec3 sideDist  = (((sign(d) * 0.5) + 0.5) + sign(d) * (vec3(mapPos) - o)) * deltaDist;

  bvec3 mask;
  for (int i = 0; i <= MAX_DDA_ITERATION; i++) {
    ivec3 lookupPos = mapPos + ivec3((sceneInfoBuffer.data.chunksDim - 1) / 2);

    if (i >= skippingNum && hasChunk(lookupPos)) {
      oHitChunkPosOffset    = mapPos;
      oHitChunkLookupOffset = lookupPos;
      return true;
    }

    if (i == MAX_DDA_ITERATION) {
      mask = bvec3(false);
      return false;
    }

    // hereby "branchless"
    mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
    sideDist += vec3(mask) * deltaDist;
    mapPos += ivec3(vec3(mask)) * rayStep;
  }
}
#endif