#ifndef CHUNKS_BUFFER_LAYOUT_GLSL
#define CHUNKS_BUFFER_LAYOUT_GLSL

uint getChunksBufferLinearIndex(uvec3 chunkIndex, uvec3 chunksDim) {
  return chunkIndex.x + chunkIndex.y * chunksDim.x + chunkIndex.z * chunksDim.x * chunksDim.y;
}

#endif // CHUNKS_BUFFER_LAYOUT_GLSL