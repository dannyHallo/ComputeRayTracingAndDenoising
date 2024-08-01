#ifndef CHUNK_INDEX_GLSL
#define CHUNK_INDEX_GLSL

uint getChunksBufferLinearIndex(uvec3 chunkIndex, uvec3 chunksDim) {
  return chunkIndex.x + chunkIndex.y * chunksDim.x + chunkIndex.z * chunksDim.x * chunksDim.y;
}

#endif // CHUNK_INDEX_GLSL
