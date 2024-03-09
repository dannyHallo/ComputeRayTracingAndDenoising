#ifndef SVO_BUILDER_DESCRIPTOR_SET_GLSL
#define SVO_BUILDER_DESCRIPTOR_SET_GLSL

#include "../include/svoBuilderDataStructs.glsl"

layout(binding = 6, r32f) uniform image3D chunkFieldImage;
layout(binding = 8, r32ui) uniform uimage3D chunksImage;

layout(std430, binding = 7) writeonly buffer IndirectFragLengthBuffer {
  G_IndirectDispatchInfo data;
}
indirectFragLengthBuffer;
layout(std430, binding = 0) buffer CounterBuffer { uint data; }
counterBuffer;
layout(std430, binding = 1) buffer OctreeBuffer { uint data[]; }
octreeBuffer;
layout(std430, binding = 2) buffer FragmentListBuffer { G_FragmentListEntry datas[]; }
fragmentListBuffer;
layout(std430, binding = 3) buffer BuildInfoBuffer { G_BuildInfo data; }
buildInfoBuffer;
layout(std430, binding = 4) writeonly buffer IndirectAllocNumBuffer { G_IndirectDispatchInfo data; }
indirectAllocNumBuffer;
layout(std430, binding = 5) buffer FragmentListInfoBuffer { G_FragmentListInfo data; }
fragmentListInfoBuffer;
layout(std430, binding = 9) buffer ChunksInfoBuffer { G_ChunksInfo data; }
chunksInfoBuffer;
layout(std430, binding = 10) buffer OctreeBufferUsedSizeInfoBuffer { uint data; }
octreeBufferUsedSizeInfoBuffer;

#endif // SVO_BUILDER_DESCRIPTOR_SET_GLSL