#ifndef SVO_BUILDER_DATA_STRUCTS_GLSL
#define SVO_BUILDER_DATA_STRUCTS_GLSL

struct G_ChunksInfo {
  uvec3 chunksDim;
  uvec3 currentlyWritingChunk;
};

struct G_FragmentListEntry {
  uint coordinates;
  uint properties;
};

struct G_BuildInfo {
  uint allocBegin;
  uint allocNum;
};

struct G_IndirectDispatchInfo {
  uint dispatchX;
  uint dispatchY;
  uint dispatchZ;
};

struct G_FragmentListInfo {
  uint voxelResolution;
  uint voxelFragmentCount;
};

struct G_OctreeBufferSizeInfo {
  uint octreeBufferSize;
  uint octreeBufferSizeToLastChunk;
};

#endif // SVO_BUILDER_DATA_STRUCTS_GLSL