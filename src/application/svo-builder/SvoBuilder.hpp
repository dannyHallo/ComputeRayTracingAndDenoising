#pragma once

#include "custom-mem-alloc/CustomMemoryAllocator.hpp"
#include "scheduler/Scheduler.hpp"
#include "volk.h"

#include "glm/glm.hpp" // IWYU pragma: export

#include <memory>
#include <unordered_map>

struct ConfigContainer;

class DescriptorSetBundle;
class ComputePipeline;
class Logger;
class VulkanApplicationContext;
class Buffer;
class Image;
class ShaderCompiler;
class ShaderChangeListener;

class SvoBuilder : public PipelineScheduler {
private:
  struct ChunkIndex {
    uint32_t x;
    uint32_t y;
    uint32_t z;

    bool operator==(const ChunkIndex &other) const {
      return x == other.x && y == other.y && z == other.z;
    }
  };

  struct ChunkIndexHash {
    std::size_t operator()(const ChunkIndex &ci) const {
      std::size_t hx = std::hash<uint32_t>()(ci.x);
      std::size_t hy = std::hash<uint32_t>()(ci.y);
      std::size_t hz = std::hash<uint32_t>()(ci.z);

      return hx ^ (hy << 1) ^ (hz << 2);
    }
  };

public:
  SvoBuilder(VulkanApplicationContext *appContext, Logger *logger, ShaderCompiler *shaderCompiler,
             ShaderChangeListener *shaderChangeListener, ConfigContainer *configContainer);
  ~SvoBuilder() override;

  // disable copy and move
  SvoBuilder(SvoBuilder const &)            = delete;
  SvoBuilder(SvoBuilder &&)                 = delete;
  SvoBuilder &operator=(SvoBuilder const &) = delete;
  SvoBuilder &operator=(SvoBuilder &&)      = delete;

  void init();
  void onPipelineRebuilt() override;

  void buildScene();

  void handleCursorHit(glm::vec3 hitPos, bool deletionMode);

  Buffer *getAppendedOctreeBuffer() { return _appendedOctreeBuffer.get(); }
  Buffer *getChunkIndicesBuffer() { return _chunkIndicesBuffer.get(); }

  [[nodiscard]] uint32_t getVoxelLevelCount() const { return _voxelLevelCount; }
  [[nodiscard]] glm::uvec3 getChunksDim() const;

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  ShaderCompiler *_shaderCompiler;
  ShaderChangeListener *_shaderChangeListener;

  ConfigContainer *_configContainer;

  uint32_t _voxelLevelCount = 0;

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;
  std::unique_ptr<CustomMemoryAllocator> _chunkBufferMemoryAllocator;

  VkCommandBuffer _octreeCreationCommandBuffer = VK_NULL_HANDLE;

  std::vector<ChunkIndex> _getEditingChunks(glm::vec3 centerPos, float radius);

  void _recordCommandBuffers();
  void _recordOctreeCreationCommandBuffer();

  void _editExistingChunk(ChunkIndex chunkIndex);
  void _buildChunkFromNoise(ChunkIndex chunkIndex);

  /// IMAGES
  std::unique_ptr<Image> _chunkFieldImage;
  std::unordered_map<ChunkIndex, std::unique_ptr<Image>, ChunkIndexHash>
      _chunkIndexToFieldImagesMap;
  std::unordered_map<ChunkIndex, CustomMemoryAllocationResult, ChunkIndexHash>
      _chunkIndexToBufferAllocResult;
  void _createImages();

  /// BUFFERS
  std::unique_ptr<Buffer> _chunkIndicesBuffer;
  std::unique_ptr<Buffer> _appendedOctreeBuffer;
  std::unique_ptr<Buffer> _chunksInfoBuffer;
  std::unique_ptr<Buffer> _chunkEditingInfoBuffer;
  std::unique_ptr<Buffer> _octreeBufferLengthBuffer;
  std::unique_ptr<Buffer> _octreeBufferWriteOffsetBuffer;

  std::unique_ptr<Buffer> _indirectFragLengthBuffer;
  std::unique_ptr<Buffer> _counterBuffer;
  std::unique_ptr<Buffer> _chunkOctreeBuffer;
  std::unique_ptr<Buffer> _fragmentListBuffer;
  std::unique_ptr<Buffer> _octreeBuildInfoBuffer;
  std::unique_ptr<Buffer> _indirectAllocNumBuffer;
  std::unique_ptr<Buffer> _fragmentListInfoBuffer;

  void _createBuffers(size_t octreeBufferSize);
  void _initBufferData();
  void _resetBufferDataForNewChunkGeneration(ChunkIndex chunkIndex);

  /// PIPELINES

  std::unique_ptr<ComputePipeline> _chunkIndicesBufferUpdaterPipeline;

  std::unique_ptr<ComputePipeline> _chunkFieldConstructionPipeline;
  std::unique_ptr<ComputePipeline> _chunkFieldModificationPipeline;
  std::unique_ptr<ComputePipeline> _chunkVoxelCreationPipeline;
  std::unique_ptr<ComputePipeline> _chunkModifyArgPipeline;

  std::unique_ptr<ComputePipeline> _initNodePipeline;
  std::unique_ptr<ComputePipeline> _tagNodePipeline;
  std::unique_ptr<ComputePipeline> _allocNodePipeline;
  std::unique_ptr<ComputePipeline> _modifyArgPipeline;

  void _createDescriptorSetBundle();
  void _createPipelines();
};