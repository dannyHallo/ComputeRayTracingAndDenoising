#pragma once

#include "scheduler/Scheduler.hpp"
#include "volk.h"

#include "glm/glm.hpp" // IWYU pragma: export

#include <memory>

struct VoxData;

class DescriptorSetBundle;
class ComputePipeline;
class Logger;
class VulkanApplicationContext;
class Buffer;
class Image;
class ShaderCompiler;
class ShaderChangeListener;
class TomlConfigReader;
class CustomMemoryAllocator;

class SvoBuilder : public Scheduler {
public:
  SvoBuilder(VulkanApplicationContext *appContext, Logger *logger, ShaderCompiler *shaderCompiler,
             ShaderChangeListener *shaderChangeListener, TomlConfigReader *tomlConfigReader);
  ~SvoBuilder() override;

  // disable copy and move
  SvoBuilder(SvoBuilder const &)            = delete;
  SvoBuilder(SvoBuilder &&)                 = delete;
  SvoBuilder &operator=(SvoBuilder const &) = delete;
  SvoBuilder &operator=(SvoBuilder &&)      = delete;

  void init();
  void update() override;

  void buildScene();

  Buffer *getAppendedOctreeBuffer() { return _appendedOctreeBuffer.get(); }
  Buffer *getChunksBuffer() { return _chunksBuffer.get(); }

  [[nodiscard]] uint32_t getVoxelLevelCount() const { return _voxelLevelCount; }
  [[nodiscard]] glm::uvec3 getChunksDim() const;

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  ShaderCompiler *_shaderCompiler;
  ShaderChangeListener *_shaderChangeListener;
  TomlConfigReader *_tomlConfigReader;

  // config
  uint32_t _chunkVoxelDim{};
  uint32_t _chunkDimX{};
  uint32_t _chunkDimY{};
  uint32_t _chunkDimZ{};
  void _loadConfig();

  uint32_t _voxelLevelCount = 0;

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;
  std::unique_ptr<CustomMemoryAllocator> _chunkBufferMemoryAllocator;

  VkFence _timelineFence                             = VK_NULL_HANDLE;
  VkCommandBuffer _fragmentListCreationCommandBuffer = VK_NULL_HANDLE;
  VkCommandBuffer _octreeCreationCommandBuffer       = VK_NULL_HANDLE;

  void _recordCommandBuffers();
  void _recordFragmentListCreationCommandBuffer();
  void _recordOctreeCreationCommandBuffer();

  void _buildChunk(glm::uvec3 currentlyWritingChunk);

  void _createFence();

  /// IMAGES
  std::unique_ptr<Image> _chunkFieldImage;
  void _createImages();

  /// BUFFERS
  std::unique_ptr<Buffer> _chunksBuffer;
  std::unique_ptr<Buffer> _appendedOctreeBuffer;
  std::unique_ptr<Buffer> _chunksInfoBuffer;
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
  void _resetBufferDataForNewChunkGeneration(glm::uvec3 currentlyWritingChunk);

  /// PIPELINES

  std::unique_ptr<ComputePipeline> _chunksBuilderPipeline;

  std::unique_ptr<ComputePipeline> _chunkFieldConstructionPipeline;
  std::unique_ptr<ComputePipeline> _chunkVoxelCreationPipeline;
  std::unique_ptr<ComputePipeline> _chunkModifyArgPipeline;

  std::unique_ptr<ComputePipeline> _initNodePipeline;
  std::unique_ptr<ComputePipeline> _tagNodePipeline;
  std::unique_ptr<ComputePipeline> _allocNodePipeline;
  std::unique_ptr<ComputePipeline> _modifyArgPipeline;

  void _createDescriptorSetBundle();
  void _createPipelines();
};