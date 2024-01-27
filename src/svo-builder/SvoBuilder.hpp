#pragma once

#include "SvoBuilderBufferStructs.hpp"

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

  Buffer *getOctreeBuffer() { return _octreeBuffer.get(); }
  Buffer *getPaletteBuffer() { return _paletteBuffer.get(); }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

  std::unique_ptr<ComputePipeline> _initNodePipeline;
  std::unique_ptr<ComputePipeline> _tagNodePipeline;
  std::unique_ptr<ComputePipeline> _allocNodePipeline;
  std::unique_ptr<ComputePipeline> _modifyArgPipeline;

  void _createDescriptorSetBundle();
  void _createPipelines();

  // BUFFERS

  std::unique_ptr<Buffer> _paletteBuffer; // filled initially

  std::unique_ptr<Buffer> _atomicCounterBuffer;
  std::unique_ptr<Buffer> _octreeBuffer;
  std::unique_ptr<Buffer> _fragmentListBuffer; // filled initially
  std::unique_ptr<Buffer> _buildInfoBuffer;
  std::unique_ptr<Buffer> _indirectBuffer;

  void _createBuffers(VoxData const &voxData);
  void _initBufferData(VoxData const &voxData);
};