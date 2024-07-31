#ifndef CASCADED_MARCHING_GLSL
#define CASCADED_MARCHING_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/chunksBufferLayout.glsl"
#include "../include/ddaMarching.glsl"
#include "../include/svoMarching.glsl"

struct MarchingResult {
  float t;
  uint iter;
  uint chunkTraversed;
  vec3 color;
  vec3 position;
  vec3 nextTracingPosition;
  vec3 normal;
  uint voxHash;
  bool lightSourceHit;
};

// this marching algorithm fetches leaf properties
bool cascadedMarching(out MarchingResult oResult, vec3 o, vec3 d) {
  ivec3 hitChunkOffset;
  uvec3 hitChunkLookupOffset;
  bool hitVoxel = false;

  oResult.iter           = 0;
  oResult.chunkTraversed = 0;
  oResult.color          = vec3(0);

  d = max(abs(d), vec3(kEpsilon)) * (step(0.0, d) * 2.0 - 1.0);

  ivec3 mapPos               = ivec3(floor(o));
  const vec3 deltaDist       = 1.0 / abs(d);
  const ivec3 rayStep        = ivec3(sign(d));
  vec3 sideDist              = (((sign(d) * 0.5) + 0.5) + sign(d) * (vec3(mapPos) - o)) * deltaDist;
  bool enteredBigBoundingBox = false;
  uint ddaIteration          = 0;
  while (ddaMarchingWithSave(hitChunkOffset, hitChunkLookupOffset, mapPos, sideDist,
                             enteredBigBoundingBox, ddaIteration, deltaDist, rayStep, o, d)) {
    // preOffset is to offset the octree tracing position, which works best with the range of [1, 2]
    const ivec3 preOffset   = ivec3(1);
    const vec3 originOffset = preOffset - hitChunkOffset;

    uint chunkBufferOffset =
        chunkIndicesBuffer.data[getChunksBufferLinearIndex(hitChunkLookupOffset, sceneInfoBuffer.data.chunksDim)] - 1;

    uint chunkIterCount;
    hitVoxel = svoMarching(oResult.t, chunkIterCount, oResult.color, oResult.position,
                           oResult.nextTracingPosition, oResult.normal, oResult.voxHash,
                           oResult.lightSourceHit, o + originOffset, d, chunkBufferOffset);

    oResult.position -= originOffset;
    oResult.nextTracingPosition -= originOffset;

    oResult.iter += chunkIterCount;
    oResult.chunkTraversed++;

    if (hitVoxel) {
      break;
    }
  }

  return hitVoxel;
}

#endif // CASCADED_MARCHING_GLSL