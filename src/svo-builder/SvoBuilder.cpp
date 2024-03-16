#include "SvoBuilder.hpp"

#include "file-watcher/ShaderChangeListener.hpp"
#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "pipeline/ComputePipeline.hpp"
#include "pipeline/DescriptorSetBundle.hpp"
#include "svo-builder/VoxLoader.hpp"
#include "utils/config/RootDir.h"
#include "utils/file-io/ShaderFileReader.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/toml-config/TomlConfigReader.hpp"
#include "utils/vulkan/SimpleCommands.hpp"

#include <chrono>
#include <cmath>

// TODO: change 3D image into a flattened storage buffer, to support 1x1x1 chunk

namespace {

// VoxData _fetchVoxData() {
//   std::string const kFileName      = "128_monu2_light";
//   std::string const kPathToVoxFile = kPathToResourceFolder + "models/vox/" + kFileName + ".vox";
//   return VoxLoader::fetchDataFromFile(kPathToVoxFile);
// }

// the worst case: for every fragment, we assume they all reside in different parents, in the
// current building algorithm, a non-leaf node's children must be initialized (to zero if empty at
// last), however, when the estimated amount of nodes exceeds the actual amount, the size of the
// octree is fixed.
// https://eisenwave.github.io/voxel-compression-docs/svo/svo.html

// uint32_t _getMaximumNodeCountOfOctree(VoxData const &voxData) {
//   uint32_t maximumSize = 0;

//   uint32_t res                     = voxData.voxelResolution;
//   uint32_t elementCount            = voxData.fragmentList.size();
//   uint32_t maximumNodeCountAtLevel = 0;

//   while (res != 1) {
//     maximumNodeCountAtLevel = std::min(elementCount * 8, res * res * res);
//     maximumSize += maximumNodeCountAtLevel;
//     res          = res >> 1;
//     elementCount = std::ceil(static_cast<float>(elementCount) / 8);
//   }

//   return maximumSize;
// }

std::string _makeShaderFullPath(std::string const &shaderName) {
  return kPathToResourceFolder + "shaders/svo-builder/" + shaderName;
}
} // namespace

SvoBuilder::SvoBuilder(VulkanApplicationContext *appContext, Logger *logger,
                       ShaderCompiler *shaderCompiler, ShaderChangeListener *shaderChangeListener,
                       TomlConfigReader *tomlConfigReader)
    : _appContext(appContext), _logger(logger), _shaderCompiler(shaderCompiler),
      _shaderChangeListener(shaderChangeListener), _tomlConfigReader(tomlConfigReader) {
  _loadConfig();
  _createFence();
}

SvoBuilder::~SvoBuilder() {
  vkDestroyFence(_appContext->getDevice(), _timelineFence, nullptr);
  vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                       &_fragmentListCreationCommandBuffer);
  vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                       &_octreeCreationCommandBuffer);
}

void SvoBuilder::_loadConfig() {
  _chunkVoxelDim = _tomlConfigReader->getConfig<uint32_t>("SvoBuilder.chunkVoxelDim");
  _chunkDimX     = _tomlConfigReader->getConfig<uint32_t>("SvoBuilder.chunkDimX");
  _chunkDimY     = _tomlConfigReader->getConfig<uint32_t>("SvoBuilder.chunkDimY");
  _chunkDimZ     = _tomlConfigReader->getConfig<uint32_t>("SvoBuilder.chunkDimZ");
}

void SvoBuilder::_createFence() {
  VkFenceCreateInfo fenceCreateInfoNotSignalled{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  // VkFenceCreateInfo fenceCreateInfoPreSignalled{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  // fenceCreateInfoPreSignalled.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(_appContext->getDevice(), &fenceCreateInfoNotSignalled, nullptr, &_timelineFence);
}

glm::uvec3 SvoBuilder::getChunksDim() const { return {_chunkDimX, _chunkDimY, _chunkDimZ}; }

void SvoBuilder::init() {
  _voxelLevelCount = static_cast<uint32_t>(std::log2(_chunkVoxelDim));

  // images
  _createImages();

  // buffers
  _createBuffers();

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();
  _recordCommandBuffers();
}

void SvoBuilder::update() {
  // _initBufferData(); // FIXME: update this logic so we can update the shaders at runtime
  _recordCommandBuffers();
}

// call me every time before building a new chunk
void SvoBuilder::_resetBufferDataForNewChunkGeneration(glm::uvec3 currentlyWritingChunk) {
  uint32_t atomicCounterInitData = 1;
  _counterBuffer->fillData(&atomicCounterInitData);

  G_OctreeBuildInfo buildInfo{};
  buildInfo.allocBegin = 0;
  buildInfo.allocNum   = 8;
  _octreeBuildInfoBuffer->fillData(&buildInfo);

  G_IndirectDispatchInfo indirectDispatchInfo{};
  indirectDispatchInfo.dispatchX = 1;
  indirectDispatchInfo.dispatchY = 1;
  indirectDispatchInfo.dispatchZ = 1;
  _indirectAllocNumBuffer->fillData(&indirectDispatchInfo);
  _indirectFragLengthBuffer->fillData(&indirectDispatchInfo);

  G_FragmentListInfo fragmentListInfo{};
  fragmentListInfo.voxelResolution    = _chunkVoxelDim;
  fragmentListInfo.voxelFragmentCount = 0;
  _fragmentListInfoBuffer->fillData(&fragmentListInfo);

  G_ChunksInfo chunksInfo{};
  chunksInfo.chunksDim             = getChunksDim();
  chunksInfo.currentlyWritingChunk = currentlyWritingChunk;
  _chunksInfoBuffer->fillData(&chunksInfo);

  // the first 8 are not calculated, so pre-allocate them
  uint32_t octreeBufferSize = 8;
  _octreeBufferLengthBuffer->fillData(&octreeBufferSize);
}

void SvoBuilder::buildScene() {
  uint32_t zero = 0;
  _octreeBufferAccumLengthBuffer->fillData(&zero);

  uint32_t minTimeMs = std::numeric_limits<uint32_t>::max();
  uint32_t maxTimeMs = 0;
  uint32_t avgTimeMs = 0;
  for (uint32_t z = 0; z < _chunkDimZ; z++) {
    for (uint32_t y = 0; y < _chunkDimY; y++) {
      for (uint32_t x = 0; x < _chunkDimX; x++) {
        auto start = std::chrono::steady_clock::now();
        _buildChunk({x, y, z});
        auto end      = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        minTimeMs     = std::min(minTimeMs, static_cast<uint32_t>(duration));
        maxTimeMs     = std::max(maxTimeMs, static_cast<uint32_t>(duration));
        avgTimeMs += static_cast<uint32_t>(duration);
      }
    }
  }
  avgTimeMs /= _chunkDimX * _chunkDimY * _chunkDimZ;

  _logger->info("min time: {} ms, max time: {} ms, avg time: {} ms", minTimeMs, maxTimeMs,
                avgTimeMs);

  _logger->info("build done! used octree buffer size: {} mb",
                static_cast<float>(sizeof(uint32_t) * _octreeBufferAccumLength) / (1024 * 1024));
}

void SvoBuilder::_buildChunk(glm::uvec3 currentlyWritingChunk) {
  _resetBufferDataForNewChunkGeneration(currentlyWritingChunk);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  // wait until the image is ready
  submitInfo.waitSemaphoreCount = 0;
  // signal a semaphore after excecution finished
  submitInfo.signalSemaphoreCount = 0;
  // wait for no stage
  VkPipelineStageFlags waitStages{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
  submitInfo.pWaitDstStageMask = &waitStages;

  // step 1: fragment list creation
  std::vector<VkCommandBuffer> commandBuffersToSubmit1{_fragmentListCreationCommandBuffer};
  submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersToSubmit1.size());
  submitInfo.pCommandBuffers    = commandBuffersToSubmit1.data();

  vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo, _timelineFence);

  vkWaitForFences(_appContext->getDevice(), 1, &_timelineFence, VK_TRUE, UINT64_MAX);
  vkResetFences(_appContext->getDevice(), 1, &_timelineFence);

  // intermediate step: check if the fragment list is empty, if so, we can skip the octree creation
  G_FragmentListInfo fragmentListInfo{};
  _fragmentListInfoBuffer->fetchData(&fragmentListInfo);
  if (fragmentListInfo.voxelFragmentCount == 0) {
    return;
  }

  // step 2: octree construction (optional)
  std::vector<VkCommandBuffer> commandBuffersToSubmit2{_octreeCreationCommandBuffer};
  submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersToSubmit2.size());
  submitInfo.pCommandBuffers    = commandBuffersToSubmit2.data();

  vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo, _timelineFence);

  vkWaitForFences(_appContext->getDevice(), 1, &_timelineFence, VK_TRUE, UINT64_MAX);
  vkResetFences(_appContext->getDevice(), 1, &_timelineFence);

  // after the fence, all work submitted to GPU has been finished, and we can read the buffer size
  // data back from the staging buffer

  uint32_t octreeBufferLength = 0;
  _octreeBufferLengthBuffer->fetchData(&octreeBufferLength);

  VkCommandBuffer commandBuffer =
      beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());

  VkBufferCopy bufCopy = {
      0,                                           // srcOffset
      _octreeBufferAccumLength * sizeof(uint32_t), // dstOffset,
      octreeBufferLength * sizeof(uint32_t),       // size
  };

  // copy staging buffer to main buffer
  vkCmdCopyBuffer(commandBuffer, _chunkOctreeBuffer->getVkBuffer(),
                  _appendedOctreeBuffer->getVkBuffer(), 1, &bufCopy);

  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), commandBuffer);

  // update accum length
  _octreeBufferAccumLength += octreeBufferLength;
  _octreeBufferAccumLengthBuffer->fillData(&_octreeBufferAccumLength);
}

void SvoBuilder::_createImages() {
  _chunkFieldImage =
      std::make_unique<Image>(_chunkVoxelDim + 1, _chunkVoxelDim + 1, _chunkVoxelDim + 1,
                              VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _chunksImage = std::make_unique<Image>(_chunkDimX, _chunkDimY, _chunkDimZ, VK_FORMAT_R32_UINT,
                                         VK_IMAGE_USAGE_STORAGE_BIT);
}

// voxData is passed in to decide the size of some buffers dureing allocation
void SvoBuilder::_createBuffers() {
  _counterBuffer = std::make_unique<Buffer>(sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            MemoryStyle::kDedicated);

  uint32_t sizeInWorstCase =
      std::ceil(static_cast<float>(_chunkVoxelDim * _chunkVoxelDim * _chunkVoxelDim) *
                sizeof(uint32_t) * 8.F / 7.F);

  _logger->info("estimated chunk staging buffer size : {} mb",
                static_cast<float>(sizeInWorstCase) / (1024 * 1024));

  _chunkOctreeBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t) * _chunkVoxelDim * _chunkVoxelDim * _chunkVoxelDim,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      MemoryStyle::kHostVisible);

  uint32_t gb                      = 1024 * 1024 * 1024;
  uint32_t maximumOctreeBufferSize = 2 * gb;
  _indirectFragLengthBuffer        = std::make_unique<Buffer>(sizeof(G_IndirectDispatchInfo),
                                                       VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                       MemoryStyle::kDedicated);

  _appendedOctreeBuffer = std::make_unique<Buffer>(maximumOctreeBufferSize,
                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                   MemoryStyle::kDedicated);

  uint32_t maximumFragmentListBufferSize =
      sizeof(G_FragmentListEntry) * _chunkVoxelDim * _chunkVoxelDim * _chunkVoxelDim;
  _fragmentListBuffer = std::make_unique<Buffer>(
      maximumFragmentListBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _logger->info("fragment list buffer size: {} mb",
                static_cast<float>(maximumFragmentListBufferSize) / (1024 * 1024));

  _octreeBuildInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_OctreeBuildInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _indirectAllocNumBuffer = std::make_unique<Buffer>(sizeof(G_IndirectDispatchInfo),
                                                     VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                     MemoryStyle::kDedicated);

  _fragmentListInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_FragmentListInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _chunksInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_ChunksInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _octreeBufferLengthBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _octreeBufferAccumLengthBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);
}

void SvoBuilder::_createDescriptorSetBundle() {
  _descriptorSetBundle =
      std::make_unique<DescriptorSetBundle>(_appContext, 1, VK_SHADER_STAGE_COMPUTE_BIT);

  _descriptorSetBundle->bindStorageImage(6, _chunkFieldImage.get());
  _descriptorSetBundle->bindStorageImage(8, _chunksImage.get());

  _descriptorSetBundle->bindStorageBuffer(7, _indirectFragLengthBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(0, _counterBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(1, _chunkOctreeBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(2, _fragmentListBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(3, _octreeBuildInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(4, _indirectAllocNumBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(5, _fragmentListInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(9, _chunksInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(10, _octreeBufferLengthBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(11, _octreeBufferAccumLengthBuffer.get());

  _descriptorSetBundle->create();
}

void SvoBuilder::_createPipelines() {
  _chunksBuilderPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunksBuilder.comp"), WorkGroupSize{8, 8, 8},
      _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener, true);
  _chunksBuilderPipeline->compileAndCacheShaderModule(false);
  _chunksBuilderPipeline->build();

  _chunkFieldConstructionPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunkFieldConstruction.comp"),
      WorkGroupSize{8, 8, 8}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener,
      true);
  _chunkFieldConstructionPipeline->compileAndCacheShaderModule(false);
  _chunkFieldConstructionPipeline->build();

  _chunkVoxelCreationPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunkVoxelCreation.comp"),
      WorkGroupSize{8, 8, 8}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener,
      true);
  _chunkVoxelCreationPipeline->compileAndCacheShaderModule(false);
  _chunkVoxelCreationPipeline->build();

  _chunkModifyArgPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunkModifyArg.comp"),
      WorkGroupSize{1, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener,
      true);
  _chunkModifyArgPipeline->compileAndCacheShaderModule(false);
  _chunkModifyArgPipeline->build();

  _initNodePipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("octreeInitNode.comp"),
      WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener,
      true);
  _initNodePipeline->compileAndCacheShaderModule(false);
  _initNodePipeline->build();

  _tagNodePipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("octreeTagNode.comp"),
      WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener,
      true);
  _tagNodePipeline->compileAndCacheShaderModule(false);
  _tagNodePipeline->build();

  _allocNodePipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("octreeAllocNode.comp"),
      WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener,
      true);
  _allocNodePipeline->compileAndCacheShaderModule(false);
  _allocNodePipeline->build();

  _modifyArgPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("octreeModifyArg.comp"),
      WorkGroupSize{1, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener,
      true);
  _modifyArgPipeline->compileAndCacheShaderModule(false);
  _modifyArgPipeline->build();
}

void SvoBuilder::_recordCommandBuffers() {
  _recordFragmentListCreationCommandBuffer();
  _recordOctreeCreationCommandBuffer();
}

void SvoBuilder::_recordFragmentListCreationCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = _appContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo,
                           &_fragmentListCreationCommandBuffer);

  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(_fragmentListCreationCommandBuffer, &beginInfo);

  // create the standard memory barrier
  VkMemoryBarrier shaderAccessBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  shaderAccessBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  shaderAccessBarrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  _chunkFieldConstructionPipeline->recordCommand(_fragmentListCreationCommandBuffer, 0,
                                                 _chunkVoxelDim + 1, _chunkVoxelDim + 1,
                                                 _chunkVoxelDim + 1);

  vkCmdPipelineBarrier(_fragmentListCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0, nullptr,
                       0, nullptr);

  _chunkVoxelCreationPipeline->recordCommand(_fragmentListCreationCommandBuffer, 0, _chunkVoxelDim,
                                             _chunkVoxelDim, _chunkVoxelDim);

  vkCmdPipelineBarrier(_fragmentListCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0, nullptr,
                       0, nullptr);

  vkEndCommandBuffer(_fragmentListCreationCommandBuffer);
}

void SvoBuilder::_recordOctreeCreationCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = _appContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, &_octreeCreationCommandBuffer);

  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(_octreeCreationCommandBuffer, &beginInfo);

  // create the standard memory barrier
  VkMemoryBarrier shaderAccessBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  shaderAccessBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  shaderAccessBarrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  VkMemoryBarrier indirectReadBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  indirectReadBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  indirectReadBarrier.dstAccessMask   = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

  _chunkModifyArgPipeline->recordCommand(_octreeCreationCommandBuffer, 0, 1, 1, 1);

  vkCmdPipelineBarrier(_octreeCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0, nullptr,
                       0, nullptr);

  // write the chunks image, according to the accumulated buffer offset
  // we should do it here, since we can cull null chunks here after the voxels are decided
  _chunksBuilderPipeline->recordCommand(_octreeCreationCommandBuffer, 0, 1, 1, 1);

  vkCmdPipelineBarrier(_octreeCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0, nullptr,
                       0, nullptr);

  vkCmdPipelineBarrier(_octreeCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 1, &indirectReadBarrier, 0, nullptr, 0, nullptr);

  // step 2: octree construction

  for (uint32_t level = 0; level < _voxelLevelCount; level++) {
    _initNodePipeline->recordIndirectCommand(_octreeCreationCommandBuffer, 0,
                                             _indirectAllocNumBuffer->getVkBuffer());
    vkCmdPipelineBarrier(_octreeCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0,
                         nullptr, 0, nullptr);

    // that indirect buffer will no longer be updated, and it is made available by the previous
    // barrier
    _tagNodePipeline->recordIndirectCommand(_octreeCreationCommandBuffer, 0,
                                            _indirectFragLengthBuffer->getVkBuffer());

    // not last level
    if (level != _voxelLevelCount - 1) {
      vkCmdPipelineBarrier(_octreeCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0,
                           nullptr, 0, nullptr);

      _allocNodePipeline->recordIndirectCommand(_octreeCreationCommandBuffer, 0,
                                                _indirectAllocNumBuffer->getVkBuffer());
      vkCmdPipelineBarrier(_octreeCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0,
                           nullptr, 0, nullptr);

      _modifyArgPipeline->recordCommand(_octreeCreationCommandBuffer, 0, 1, 1, 1);

      vkCmdPipelineBarrier(_octreeCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0,
                           nullptr, 0, nullptr);

      vkCmdPipelineBarrier(_octreeCreationCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 1, &indirectReadBarrier, 0, nullptr, 0, nullptr);
    }
  }

  vkEndCommandBuffer(_octreeCreationCommandBuffer);
}