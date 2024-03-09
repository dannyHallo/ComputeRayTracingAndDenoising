#include "SvoBuilder.hpp"

#include "file-watcher/ShaderChangeListener.hpp"
#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "pipeline/ComputePipeline.hpp"
#include "pipeline/DescriptorSetBundle.hpp"
#include "svo-builder/VoxLoader.hpp"
#include "utils/file-io/ShaderFileReader.hpp"
#include "utils/logger/Logger.hpp"

#include "utils/config/RootDir.h"

#include <cmath>

// the voxel dimension within a chunk
static uint32_t constexpr kChunkVoxelDim = 256;
// the chunk dimension, this is worth expanding
static uint32_t constexpr kChunkDim = 3;

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
  return kRootDir + "src/shaders/svo-builder/" + shaderName;
}

} // namespace

SvoBuilder::SvoBuilder(VulkanApplicationContext *appContext, Logger *logger,
                       ShaderCompiler *shaderCompiler, ShaderChangeListener *shaderChangeListener)
    : _appContext(appContext), _logger(logger), _shaderCompiler(shaderCompiler),
      _shaderChangeListener(shaderChangeListener) {
  _createFence();
}

SvoBuilder::~SvoBuilder() {
  vkDestroyFence(_appContext->getDevice(), _timelineFence, nullptr);
  vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1, &_commandBuffer);
}

void SvoBuilder::_createFence() {
  VkFenceCreateInfo fenceCreateInfoNotSignalled{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  // VkFenceCreateInfo fenceCreateInfoPreSignalled{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  // fenceCreateInfoPreSignalled.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(_appContext->getDevice(), &fenceCreateInfoNotSignalled, nullptr, &_timelineFence);
}

glm::uvec3 SvoBuilder::getChunksDim() { return {kChunkDim, kChunkDim, kChunkDim}; }

void SvoBuilder::init() {
  _voxelLevelCount = static_cast<uint32_t>(std::log2(kChunkVoxelDim));

  // images
  _createImages();

  // buffers
  _createBuffers();

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();
  _recordCommandBuffer();
}

void SvoBuilder::update() {
  // _initBufferData(); // FIXME: update this logic
  _recordCommandBuffer();
}

// call me every time before building a new chunk
void SvoBuilder::_resetBufferDataForNewChunkGeneration(glm::uvec3 currentlyWritingChunk) {
  uint32_t atomicCounterInitData = 1;
  _counterBuffer->fillData(&atomicCounterInitData);

  G_BuildInfo buildInfo{};
  buildInfo.allocBegin = 0;
  buildInfo.allocNum   = 8;
  _buildInfoBuffer->fillData(&buildInfo);

  G_IndirectDispatchInfo indirectDispatchInfo{};
  indirectDispatchInfo.dispatchX = 1;
  indirectDispatchInfo.dispatchY = 1;
  indirectDispatchInfo.dispatchZ = 1;
  _indirectAllocNumBuffer->fillData(&indirectDispatchInfo);

  G_FragmentListInfo fragmentListInfo{};
  fragmentListInfo.voxelResolution    = kChunkVoxelDim;
  fragmentListInfo.voxelFragmentCount = 0;
  _fragmentListInfoBuffer->fillData(&fragmentListInfo);

  G_ChunksInfo chunksInfo{};
  chunksInfo.chunksDim             = getChunksDim();
  chunksInfo.currentlyWritingChunk = currentlyWritingChunk;
  _chunksInfoBuffer->fillData(&chunksInfo);

  uint32_t octreeBufferSize = 0;
  _octreeBufferSizeBuffer->fillData(&octreeBufferSize);
}

void SvoBuilder::buildScene() {
  for (uint32_t x = 0; x < kChunkDim; x++) {
    for (uint32_t y = 0; y < kChunkDim; y++) {
      for (uint32_t z = 0; z < kChunkDim; z++) {
        _buildChunk({x, y, z});
      }
    }
  }
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

  std::vector<VkCommandBuffer> commandBuffersToSubmit{_commandBuffer};
  submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersToSubmit.size());
  submitInfo.pCommandBuffers    = commandBuffersToSubmit.data();

  vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo, _timelineFence);

  vkWaitForFences(_appContext->getDevice(), 1, &_timelineFence, VK_TRUE, UINT64_MAX);
  vkResetFences(_appContext->getDevice(), 1, &_timelineFence);

  // after the fence, all work submitted to GPU has been finished, and we can read the buffer size
  // data back from the staging buffer

  // map memory, copy data, unmap memory
  uint32_t octreeBufferUsedSize = 0;

  void *mappedData = nullptr;
  vmaMapMemory(VulkanApplicationContext::getInstance()->getAllocator(),
               _octreeBufferSizeStagingBuffer->getAllocation(), &mappedData);
  memcpy(&octreeBufferUsedSize, mappedData, sizeof(uint32_t));
  vmaUnmapMemory(VulkanApplicationContext::getInstance()->getAllocator(),
                 _octreeBufferSizeStagingBuffer->getAllocation());

  // log
  _logger->info("used octree buffer size: {} mb",
                static_cast<float>(sizeof(uint32_t) * octreeBufferUsedSize) / (1024 * 1024));
}

void SvoBuilder::_createImages() {
  _chunkFieldImage =
      std::make_unique<Image>(kChunkVoxelDim + 1, kChunkVoxelDim + 1, kChunkVoxelDim + 1,
                              VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _chunksImage = std::make_unique<Image>(kChunkDim, kChunkDim, kChunkDim, VK_FORMAT_R32_UINT,
                                         VK_IMAGE_USAGE_STORAGE_BIT);
}

// voxData is passed in to decide the size of some buffers dureing allocation
void SvoBuilder::_createBuffers() {
  _counterBuffer = std::make_unique<Buffer>(sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            MemoryStyle::kDedicated);

  // uint32_t estimatedOctreeBufferSize = sizeof(uint32_t) * _getMaximumNodeCountOfOctree(voxData);

  // uint32_t maximumOctreeBufferSize = std::ceil(static_cast<float>(sizeof(uint32_t)) * kChunkSize
  // *
  //                                              kChunkSize * kChunkSize * 8.F / 7.F);

  uint32_t sizeInWorstCase =
      std::ceil(static_cast<float>(kChunkVoxelDim * kChunkVoxelDim * kChunkVoxelDim) *
                sizeof(uint32_t) * 8.F / 7.F);

  _logger->info("estimated chunk staging buffer size : {} mb",
                static_cast<float>(sizeInWorstCase) / (1024 * 1024));

  _chunkSvoStagingBuffer =
      std::make_unique<Buffer>(sizeof(uint32_t) * kChunkVoxelDim * kChunkVoxelDim * kChunkVoxelDim,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryStyle::kHostVisible);

  uint32_t gb                      = 1024 * 1024 * 1024;
  uint32_t maximumOctreeBufferSize = 2 * gb;
  _indirectFragLengthBuffer        = std::make_unique<Buffer>(sizeof(G_IndirectDispatchInfo),
                                                       VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                       MemoryStyle::kDedicated);

  _octreeBuffer = std::make_unique<Buffer>(
      maximumOctreeBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  uint32_t maximumFragmentListBufferSize =
      sizeof(G_FragmentListEntry) * kChunkVoxelDim * kChunkVoxelDim * kChunkVoxelDim;
  _fragmentListBuffer = std::make_unique<Buffer>(
      maximumFragmentListBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _logger->info("fragment list buffer size: {} mb",
                static_cast<float>(maximumFragmentListBufferSize) / (1024 * 1024));

  _buildInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_BuildInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _indirectAllocNumBuffer = std::make_unique<Buffer>(sizeof(G_IndirectDispatchInfo),
                                                     VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                     MemoryStyle::kDedicated);

  _fragmentListInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_FragmentListInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _chunksInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_ChunksInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _octreeBufferSizeBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      MemoryStyle::kDedicated);

  _octreeBufferSizeStagingBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT, MemoryStyle::kHostVisible);
}

void SvoBuilder::_createDescriptorSetBundle() {
  _descriptorSetBundle =
      std::make_unique<DescriptorSetBundle>(_appContext, 1, VK_SHADER_STAGE_COMPUTE_BIT);

  _descriptorSetBundle->bindStorageImage(6, _chunkFieldImage.get());
  _descriptorSetBundle->bindStorageImage(8, _chunksImage.get());

  _descriptorSetBundle->bindStorageBuffer(7, _indirectFragLengthBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(0, _counterBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(1, _octreeBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(2, _fragmentListBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(3, _buildInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(4, _indirectAllocNumBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(5, _fragmentListInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(9, _chunksInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(10, _octreeBufferSizeBuffer.get());

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

void SvoBuilder::_recordCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = _appContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, &_commandBuffer);

  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(_commandBuffer, &beginInfo);

  // create the standard memory barrier
  VkMemoryBarrier shaderAccessBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  shaderAccessBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  shaderAccessBarrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  VkMemoryBarrier indirectReadBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  indirectReadBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  indirectReadBarrier.dstAccessMask   = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

  // step 1: field creation and voxel fragment list construction

  _chunkFieldConstructionPipeline->recordCommand(_commandBuffer, 0, kChunkVoxelDim + 1,
                                                 kChunkVoxelDim + 1, kChunkVoxelDim + 1);

  vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0, nullptr,
                       0, nullptr);

  _chunkVoxelCreationPipeline->recordCommand(_commandBuffer, 0, kChunkVoxelDim, kChunkVoxelDim,
                                             kChunkVoxelDim);

  vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0, nullptr,
                       0, nullptr);

  _chunkModifyArgPipeline->recordCommand(_commandBuffer, 0, 1, 1, 1);

  vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0, nullptr,
                       0, nullptr);

  vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 1, &indirectReadBarrier, 0, nullptr, 0, nullptr);

  // step 2: octree construction

  for (uint32_t level = 0; level < _voxelLevelCount; level++) {
    _initNodePipeline->recordIndirectCommand(_commandBuffer, 0,
                                             _indirectAllocNumBuffer->getVkBuffer());
    vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0,
                         nullptr, 0, nullptr);

    // that indirect buffer will no longer be updated, and it is made available by the previous
    // barrier
    _tagNodePipeline->recordIndirectCommand(_commandBuffer, 0,
                                            _indirectFragLengthBuffer->getVkBuffer());

    // not last level
    if (level != _voxelLevelCount - 1) {
      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0,
                           nullptr, 0, nullptr);

      _allocNodePipeline->recordIndirectCommand(_commandBuffer, 0,
                                                _indirectAllocNumBuffer->getVkBuffer());
      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0,
                           nullptr, 0, nullptr);

      _modifyArgPipeline->recordCommand(_commandBuffer, 0, 1, 1, 1);

      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0,
                           nullptr, 0, nullptr);

      if (level == _voxelLevelCount - 2) {
        VkMemoryBarrier transferReadBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        transferReadBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
        transferReadBarrier.dstAccessMask   = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &transferReadBarrier, 0, nullptr,
                             0, nullptr);

        VkBufferCopy bufCopy = {
            0,                                  // srcOffset
            0,                                  // dstOffset,
            _octreeBufferSizeBuffer->getSize(), // size
        };

        // this step doesn't need syncronization here, since we have fences
        vkCmdCopyBuffer(_commandBuffer, _octreeBufferSizeBuffer->getVkBuffer(),
                        _octreeBufferSizeStagingBuffer->getVkBuffer(), 1, &bufCopy);
      }

      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 1, &indirectReadBarrier, 0, nullptr, 0, nullptr);
    }
  }

  // step 3 (temporary): build the chunks

  _chunksBuilderPipeline->recordCommand(_commandBuffer, 0, kChunkDim, kChunkDim, kChunkDim);

  vkEndCommandBuffer(_commandBuffer);
}