#ifndef CHUNK_INDEX_GLSL
#define CHUNK_INDEX_GLSL

ivec3 getChunkCornerOffset() {
  return ivec3(chunksInfoBuffer.data.currentlyWritingChunk) -
         ivec3((chunksInfoBuffer.data.chunksDim - 1) / 2);
}

vec3 getFieldNodePosition(ivec3 chunkPosition, ivec3 uvi) {
  return vec3(chunkPosition) + vec3(uvi) / float(fragmentListInfoBuffer.data.voxelResolution);
}

#endif // CHUNK_INDEX_GLSL
