#pragma once

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

  Buffer *getVoxelBuffer() { return _voxelBuffer.get(); }
  Buffer *getPaletteBuffer() { return _paletteBuffer.get(); }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  std::vector<uint32_t> _voxelData;
  std::array<uint32_t, 256> _paletteData{}; // NOLINT

  std::unique_ptr<VoxData> _voxData;

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

  std::unique_ptr<ComputePipeline> _initNodePipeline;
  std::unique_ptr<ComputePipeline> _tagNodePipeline;
  std::unique_ptr<ComputePipeline> _allocNodePipeline;
  std::unique_ptr<ComputePipeline> _modifyArgPipeline;

  std::unique_ptr<Buffer> _voxelBuffer;
  std::unique_ptr<Buffer> _paletteBuffer;

  void _fetchRawVoxelData(size_t index);
  void _createVoxelData();
  void _createBuffers();

  void _createDescriptorSetBundle();
  void _createPipelines();
};