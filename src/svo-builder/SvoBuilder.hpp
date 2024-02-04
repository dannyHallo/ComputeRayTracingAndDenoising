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

  std::unique_ptr<ComputePipeline> _initNodePipeline;
  std::unique_ptr<ComputePipeline> _tagNodePipeline;
  std::unique_ptr<ComputePipeline> _allocNodePipeline;
  std::unique_ptr<ComputePipeline> _modifyArgPipeline;

  VkCommandBuffer _commandBuffer = VK_NULL_HANDLE; // only one command buffer is needed
  void _recordCommandBuffer(uint32_t voxelFragmentCount, uint32_t octreeLevelCount);

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

  void _createDescriptorSetBundle();
  void _createPipelines();
};