#ifndef SVO_BUILDER_DESCRIPTOR_SET_GLSL
#define SVO_BUILDER_DESCRIPTOR_SET_GLSL

struct FragmentListEntry {
  uint coordinates;
  uint properties;
};

layout(std430, binding = 0) buffer uuCounter { uint uCounter; };
layout(std430, binding = 1) buffer uuOctree { uint uOctree[]; };
layout(std430, binding = 2) readonly buffer uuFragmentList { FragmentListEntry uFragmentList[]; };
layout(std430, binding = 3) buffer uuBuildInfo { uint uAllocBegin, uAllocNum; };
layout(std430, binding = 4) writeonly buffer uuIndirect {
  uint uNumGroupX, uNumGroupY, uNumGroupZ;
};
layout(std430, binding = 5) readonly buffer uuFragmentData {
  uint voxelResolution, voxelFragmentCount;
};

#endif // SVO_BUILDER_DESCRIPTOR_SET_GLSL