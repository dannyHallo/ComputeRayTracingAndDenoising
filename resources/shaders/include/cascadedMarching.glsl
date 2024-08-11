#ifndef CASCADED_MARCHING_GLSL
#define CASCADED_MARCHING_GLSL

#include "../include/svoTracerDescriptorSetLayouts.glsl"

#include "../include/chunking.glsl"
#include "../include/ddaMarching.glsl"
#include "../include/svoMarching.glsl"

struct MarchingResult {
  uint iter;
  uint chunkTraversed;
  float t;
  vec3 color;
  vec3 position;
  vec3 nextTracingPosition;
  vec3 normal;
  uint voxHash;
  bool lightSourceHit;
};

// this marching algorithm fetches leaf properties
bool cascadedMarching(out MarchingResult oResult, vec3 o, vec3 d) {
  ivec3 chunkIndex;
  bool hitVoxel = false;

  oResult.iter                = 0;
  oResult.chunkTraversed      = 0;
  oResult.t                   = 1e10;
  oResult.color               = vec3(0);
  oResult.position            = o + d * oResult.t;
  oResult.nextTracingPosition = oResult.position;
  oResult.normal              = vec3(0);
  oResult.voxHash             = 0;
  oResult.lightSourceHit      = false;

  d = max(abs(d), vec3(kEpsilon)) * (step(0.0, d) * 2.0 - 1.0);

  ivec3 mapPos               = ivec3(floor(o));
  const vec3 deltaDist       = 1.0 / abs(d);
  const ivec3 rayStep        = ivec3(sign(d));
  vec3 sideDist              = (((sign(d) * 0.5) + 0.5) + sign(d) * (vec3(mapPos) - o)) * deltaDist;
  bool enteredBigBoundingBox = false;
  uint ddaIteration          = 0;
  while (ddaMarchingWithSave(chunkIndex, mapPos, sideDist, enteredBigBoundingBox, ddaIteration,
                             deltaDist, rayStep, o, d)) {
    // preOffset is to offset the octree tracing position, which works best with the range of [1, 2]
    const ivec3 preOffset   = ivec3(1);
    const vec3 originOffset = preOffset - chunkIndex;

    uint chunkBufferOffset =
        chunkIndicesBuffer
            .data[getChunksBufferLinearIndex(uvec3(chunkIndex), sceneInfoBuffer.data.chunksDim)] -
        1;

    uint chunkIterCount, voxHash;
    vec3 color, pos, nextTracingPos, normal;
    bool lightSourceHit;
    float t;
    hitVoxel = svoMarching(t, chunkIterCount, color, pos, nextTracingPos, normal, voxHash,
                           lightSourceHit, o + originOffset, d, chunkBufferOffset);

    oResult.iter += chunkIterCount;
    oResult.chunkTraversed++;
    if (hitVoxel) {
      oResult.t                   = t;
      oResult.color               = color;
      oResult.position            = pos - originOffset;
      oResult.nextTracingPosition = nextTracingPos - originOffset;
      oResult.normal              = normal;
      oResult.voxHash             = voxHash;
      oResult.lightSourceHit      = lightSourceHit;
      return true;
    }
  }
  return false;
}

#endif // CASCADED_MARCHING_GLSL