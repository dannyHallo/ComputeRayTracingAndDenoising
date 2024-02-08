#ifndef SVO_BUILDER_DESCRIPTOR_SET_GLSL
#define SVO_BUILDER_DESCRIPTOR_SET_GLSL

#include "svoBuilderDataStructs.glsl"

layout(binding = 6, r32ui) uniform uimage3D fragmentListImage;

layout(std430, binding = 0) buffer CounterBuffer { uint data; }
counterBuffer;
layout(std430, binding = 1) buffer OctreeBuffer { uint data[]; }
octreeBuffer;
layout(std430, binding = 2) buffer FragmentListBuffer { G_FragmentListEntry datas[]; }
fragmentListBuffer;
layout(std430, binding = 3) buffer BuildInfoBuffer { G_BuildInfo data; }
buildInfoBuffer;
layout(std430, binding = 4) writeonly buffer IndirectDispatchInfoBuffer {
  G_IndirectDispatchInfo data;
}
indirectDispatchInfoBuffer;
layout(std430, binding = 5) readonly buffer FragmentListInfoBuffer { G_FragmentListInfo data; }
fragmentListInfoBuffer;

#endif // SVO_BUILDER_DESCRIPTOR_SET_GLSL