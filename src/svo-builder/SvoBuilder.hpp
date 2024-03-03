#pragma once

#include "scheduler/Scheduler.hpp"
#include "volk/volk.h"

#include <memory>

struct VoxData;

class DescriptorSetBundle;
class ComputePipeline;
class Logger;
class VulkanApplicationContext;
class Buffer;
class Image;
class ShaderChangeListener;

class SvoBuilder : public Scheduler {
public:
  SvoBuilder(VulkanApplicationContext *appContext, Logger *logger,
             ShaderChangeListener *shaderChangeListener);
  ~SvoBuilder() override;

  // disable copy and move
  SvoBuilder(SvoBuilder const &)            = delete;
  SvoBuilder(SvoBuilder &&)                 = delete;
  SvoBuilder &operator=(SvoBuilder const &) = delete;
  SvoBuilder &operator=(SvoBuilder &&)      = delete;

  void init();
  void update() override;

  void build(VkFence svoBuildingDoneFence);

  Buffer *getOctreeBuffer() { return _octreeBuffer.get(); }

  [[nodiscard]] uint32_t getVoxelLevelCount() const { return _voxelLevelCount; }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  ShaderChangeListener *_shaderChangeListener;

  uint32_t _voxelLevelCount = 0;

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

  VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;

  void _recordCommandBuffer();

  /// IMAGES
  std::unique_ptr<Image> _chunkFieldImage;
  std::unique_ptr<Image> _chunksImage;

  void _createImages();

  /// BUFFERS
  std::unique_ptr<Buffer> _chunksInfoBuffer;

  std::unique_ptr<Buffer> _indirectFragLengthBuffer;
  std::unique_ptr<Buffer> _counterBuffer;
  std::unique_ptr<Buffer> _octreeBuffer;
  std::unique_ptr<Buffer> _fragmentListBuffer;
  std::unique_ptr<Buffer> _buildInfoBuffer;
  std::unique_ptr<Buffer> _indirectAllocNumBuffer;
  std::unique_ptr<Buffer> _fragmentListInfoBuffer;

  void _createBuffers();
  void _initBufferData();

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