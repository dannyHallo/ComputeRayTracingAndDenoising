#pragma once

#include "SvoBuilderDataGpu.hpp"
#include "utils/incl/Vulkan.hpp"

#include <array>
#include <memory>
#include <vector>

struct VoxData;

class DescriptorSetBundle;
class ComputePipeline;
class Logger;
class VulkanApplicationContext;
class Buffer;
class Image;

class SvoBuilder {
public:
  SvoBuilder(VulkanApplicationContext *appContext, Logger *logger);
  ~SvoBuilder();

  // disable copy and move
  SvoBuilder(SvoBuilder const &)            = delete;
  SvoBuilder(SvoBuilder &&)                 = delete;
  SvoBuilder &operator=(SvoBuilder const &) = delete;
  SvoBuilder &operator=(SvoBuilder &&)      = delete;

  void init();
  void build(VkFence svoBuildingDoneFence);

  Buffer *getOctreeBuffer() { return _octreeBuffer.get(); }
  Buffer *getPaletteBuffer() { return _paletteBuffer.get(); }

  [[nodiscard]] uint32_t getVoxelLevelCount() const { return _voxelLevelCount; }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  uint32_t _voxelLevelCount = 0;

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

  VkCommandBuffer _normalCalcCommandBuffer     = VK_NULL_HANDLE;
  VkCommandBuffer _octreeBuildingCommandBuffer = VK_NULL_HANDLE;

  void _recordNormalCalcCommandBuffer(uint32_t voxelFragmentCount);
  void _recordOctreeBuildingCommandBuffer(uint32_t voxelFragmentCount, uint32_t octreeLevelCount);

  /// IMAGES
  std::unique_ptr<Image> _fragmentListImage;

  void _createImages(VoxData const &voxData);

  /// BUFFERS

  std::unique_ptr<Buffer> _paletteBuffer; // filled initially

  std::unique_ptr<Buffer> _atomicCounterBuffer;
  std::unique_ptr<Buffer> _octreeBuffer;
  std::unique_ptr<Buffer> _fragmentListBuffer;
  std::unique_ptr<Buffer> _buildInfoBuffer;
  std::unique_ptr<Buffer> _indirectDispatchInfoBuffer;
  std::unique_ptr<Buffer> _fragmentListInfoBuffer;

  void _createBuffers(VoxData const &voxData);
  void _initBufferData(VoxData const &voxData);

  /// PIPELINES

  std::unique_ptr<ComputePipeline> _fragmentListImageFillingPipeline;
  std::unique_ptr<ComputePipeline> _fragmentListNormalCalcPipeline;

  std::unique_ptr<ComputePipeline> _initNodePipeline;
  std::unique_ptr<ComputePipeline> _tagNodePipeline;
  std::unique_ptr<ComputePipeline> _allocNodePipeline;
  std::unique_ptr<ComputePipeline> _modifyArgPipeline;

  void _createDescriptorSetBundle();
  void _createPipelines();
};