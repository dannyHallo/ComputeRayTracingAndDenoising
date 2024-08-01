#ifndef SVO_BUILDER_DESCRIPTOR_SET_GLSL
#define SVO_BUILDER_DESCRIPTOR_SET_GLSL

#extension GL_EXT_shader_image_load_formatted : require

#include "../include/svoBuilderDataStructs.glsl"

layout(binding = 0) uniform uimage3D chunkFieldImage;

layout(std430, binding = 1) buffer ChunkIndicesBuffer { uint data[]; }
chunkIndicesBuffer;
layout(std430, binding = 2) writeonly buffer IndirectFragLengthBuffer {
  G_IndirectDispatchInfo data;
}
indirectFragLengthBuffer;
layout(std430, binding = 3) buffer CounterBuffer { uint data; }
counterBuffer;
layout(std430, binding = 4) buffer OctreeBuffer { uint data[]; }
octreeBuffer;
layout(std430, binding = 5) buffer FragmentListBuffer { G_FragmentListEntry datas[]; }
fragmentListBuffer;
layout(std430, binding = 6) buffer OctreeBuildInfoBuffer { G_OctreeBuildInfo data; }
octreeBuildInfoBuffer;
layout(std430, binding = 7) writeonly buffer IndirectAllocNumBuffer { G_IndirectDispatchInfo data; }
indirectAllocNumBuffer;
layout(std430, binding = 8) buffer FragmentListInfoBuffer { G_FragmentListInfo data; }
fragmentListInfoBuffer;
layout(std430, binding = 9) buffer ChunksInfoBuffer { G_ChunksInfo data; }
chunksInfoBuffer;
layout(std430, binding = 10) buffer OctreeBufferLengthBuffer { uint data; }
octreeBufferLengthBuffer;
layout(std430, binding = 11) buffer OctreeBufferWriteOffsetBuffer { uint data; }
octreeBufferWriteOffsetBuffer;

layout(std430, binding = 12) buffer ChunkEditingInfo { G_ChunkEditingInfo data; }
chunkEditingInfo;

#endif // SVO_BUILDER_DESCRIPTOR_SET_GLSL