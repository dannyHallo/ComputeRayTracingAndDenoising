#include "SvoBuilder.hpp"

#include "SvoBuilderDataGpu.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "file-watcher/ShaderChangeListener.hpp"
#include "utils/config/RootDir.h"
#include "utils/io/ShaderFileReader.hpp"
#include "utils/logger/Logger.hpp"
#include "vulkan-wrapper/descriptor-set/DescriptorSetBundle.hpp"
#include "vulkan-wrapper/memory/Buffer.hpp"
#include "vulkan-wrapper/memory/Image.hpp"
#include "vulkan-wrapper/pipeline/ComputePipeline.hpp"
#include "vulkan-wrapper/utils/SimpleCommands.hpp"

#include "config-container/ConfigContainer.hpp"
#include "config-container/sub-config/BrushInfo.hpp"
#include "config-container/sub-config/TerrainInfo.hpp"

#include <chrono>
#include <cmath>

namespace {

std::string _makeShaderFullPath(std::string const &shaderName) {
  return kPathToResourceFolder + "shaders/svo-builder/" + shaderName;
}

} // namespace

SvoBuilder::SvoBuilder(VulkanApplicationContext *appContext, Logger *logger,
                       ShaderCompiler *shaderCompiler, ShaderChangeListener *shaderChangeListener,
                       ConfigContainer *configContainer)
    : _appContext(appContext), _logger(logger), _shaderCompiler(shaderCompiler),
      _shaderChangeListener(shaderChangeListener), _configContainer(configContainer) {}

SvoBuilder::~SvoBuilder() {
  vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                       &_octreeCreationCommandBuffer);
}

glm::uvec3 SvoBuilder::getChunksDim() const { return _configContainer->terrainInfo->chunksDim; }

void SvoBuilder::init() {
  _voxelLevelCount = static_cast<uint32_t>(std::log2(_configContainer->terrainInfo->chunkVoxelDim));

  size_t constexpr kMb    = 1024 * 1024;
  size_t constexpr kGb    = 1024 * kMb;
  size_t octreeBufferSize = 2 * kGb;

  _chunkBufferMemoryAllocator = std::make_unique<CustomMemoryAllocator>(_logger, octreeBufferSize);

  // images
  _createImages();

  // buffers
  _createBuffers(octreeBufferSize);
  _initBufferData();

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();
  _recordCommandBuffers();
}

void SvoBuilder::update() {
  _recordCommandBuffers();

  _chunkBufferMemoryAllocator->freeAll();
  _chunkIndexToBufferAllocResult.clear();

  _chunkIndexToFieldImagesMap.clear();

  _initBufferData();

  buildScene();
}

// call me every time before building a new chunk
void SvoBuilder::_resetBufferDataForNewChunkGeneration(ChunkIndex chunkIndex) {
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
  fragmentListInfo.voxelResolution    = _configContainer->terrainInfo->chunkVoxelDim;
  fragmentListInfo.voxelFragmentCount = 0;
  _fragmentListInfoBuffer->fillData(&fragmentListInfo);

  G_ChunksInfo chunksInfo{};
  chunksInfo.chunksDim             = getChunksDim();
  chunksInfo.currentlyWritingChunk = {chunkIndex.x, chunkIndex.y, chunkIndex.z};
  _chunksInfoBuffer->fillData(&chunksInfo);

  // the first 8 are not calculated, so pre-allocate them
  uint32_t octreeBufferSize = 8;
  _octreeBufferLengthBuffer->fillData(&octreeBufferSize);
}

void SvoBuilder::buildScene() {
  uint32_t zero = 0;
  _octreeBufferWriteOffsetBuffer->fillData(&zero);

  uint32_t minTimeMs = std::numeric_limits<uint32_t>::max();
  uint32_t maxTimeMs = 0;
  uint32_t avgTimeMs = 0;
  for (uint32_t z = 0; z < _configContainer->terrainInfo->chunksDim.z; z++) {
    for (uint32_t y = 0; y < _configContainer->terrainInfo->chunksDim.y; y++) {
      for (uint32_t x = 0; x < _configContainer->terrainInfo->chunksDim.x; x++) {
        ChunkIndex chunkIndex{x, y, z};

        auto start = std::chrono::steady_clock::now();
        _buildChunkFromNoise(chunkIndex);
        auto end      = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        minTimeMs     = std::min(minTimeMs, static_cast<uint32_t>(duration));
        maxTimeMs     = std::max(maxTimeMs, static_cast<uint32_t>(duration));
        avgTimeMs += static_cast<uint32_t>(duration);
      }
    }
  }

  avgTimeMs /= _configContainer->terrainInfo->chunksDim.x *
               _configContainer->terrainInfo->chunksDim.y *
               _configContainer->terrainInfo->chunksDim.z;

  _logger->info("min time: {} ms, max time: {} ms, avg time: {} ms", minTimeMs, maxTimeMs,
                avgTimeMs);

  _chunkBufferMemoryAllocator->printStats();
}

std::vector<SvoBuilder::ChunkIndex> SvoBuilder::_getEditingChunks(glm::vec3 centerPos,
                                                                  float radius) {
  std::vector<ChunkIndex> chunks{};

  glm::vec3 minPos = centerPos - glm::vec3{radius, radius, radius};
  glm::vec3 maxPos = centerPos + glm::vec3{radius, radius, radius};
  glm::uvec3 minChunkIndex =
      glm::uvec3{static_cast<uint32_t>(minPos.x), static_cast<uint32_t>(minPos.y),
                 static_cast<uint32_t>(minPos.z)};
  glm::uvec3 maxChunkIndex =
      glm::uvec3{static_cast<uint32_t>(maxPos.x), static_cast<uint32_t>(maxPos.y),
                 static_cast<uint32_t>(maxPos.z)};

  // ensure min and max is in the range
  auto const &chunksDim = getChunksDim();
  minChunkIndex         = glm::max(minChunkIndex, glm::uvec3{0, 0, 0});
  maxChunkIndex =
      glm::min(maxChunkIndex, glm::uvec3{chunksDim.x - 1, chunksDim.y - 1, chunksDim.z - 1});

  for (uint32_t z = minChunkIndex.z; z <= maxChunkIndex.z; z++) {
    for (uint32_t y = minChunkIndex.y; y <= maxChunkIndex.y; y++) {
      for (uint32_t x = minChunkIndex.x; x <= maxChunkIndex.x; x++) {
        chunks.emplace_back(ChunkIndex{x, y, z});
      }
    }
  }

  // print using logger
  // _logger->info("------");
  // for (auto const &chunk : chunks) {
  //   _logger->info("editing chunk: x: {}, y: {}, z: {}", chunk.x, chunk.y, chunk.z);
  // }
  // _logger->info("------");

  // print no only
  // _logger->info("editing chunks: {}", chunks.size());

  return chunks;
}

void SvoBuilder::handleCursorHit(glm::vec3 hitPos, bool deletionMode) {
  // change chunk editing buffer
  G_ChunkEditingInfo chunkEditingInfo{};
  chunkEditingInfo.pos       = hitPos;
  chunkEditingInfo.radius    = _configContainer->brushInfo->size;
  chunkEditingInfo.operation = deletionMode ? 0U : 1U; // 0 for deletion, 1 for addition
  _chunkEditingInfoBuffer->fillData(&chunkEditingInfo);

  const auto &chunks = _getEditingChunks(hitPos, _configContainer->brushInfo->size);
  for (auto const &chunk : chunks) {
    _editExistingChunk(chunk);
  }
}

void SvoBuilder::_editExistingChunk(ChunkIndex chunkIndex) {
  _resetBufferDataForNewChunkGeneration(chunkIndex);

  VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

  // if the chunk does not have save, create it to buffer
  if (_chunkIndexToFieldImagesMap.find(chunkIndex) == _chunkIndexToFieldImagesMap.end()) {
    _logger->info("constructing new field image");
    cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
    _chunkFieldConstructionPipeline->recordCommand(
        cmdBuffer, 0, _configContainer->terrainInfo->chunkVoxelDim + 1,
        _configContainer->terrainInfo->chunkVoxelDim + 1,
        _configContainer->terrainInfo->chunkVoxelDim + 1);
    endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                          _appContext->getGraphicsQueue(), cmdBuffer);
  }
  // otherwise, load from save to buffer, caching this doesn't offer performance boost
  else {
    cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
    ImageForwardingPair f{_chunkIndexToFieldImagesMap[chunkIndex].get(),
                          _chunkFieldImage.get(),
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_GENERAL};
    f.forwardCopy(cmdBuffer);
    endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                          _appContext->getGraphicsQueue(), cmdBuffer);
  }

  // edit field image
  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  _chunkFieldModificationPipeline->recordCommand(cmdBuffer, 0,
                                                 _configContainer->terrainInfo->chunkVoxelDim + 1,
                                                 _configContainer->terrainInfo->chunkVoxelDim + 1,
                                                 _configContainer->terrainInfo->chunkVoxelDim + 1);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);

  // construct voxels into fragmentlist buffer
  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  _chunkVoxelCreationPipeline->recordCommand(
      cmdBuffer, 0, _configContainer->terrainInfo->chunkVoxelDim,
      _configContainer->terrainInfo->chunkVoxelDim, _configContainer->terrainInfo->chunkVoxelDim);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);

  // check if the fragment list is empty, if so, we can skip the octree creation
  G_FragmentListInfo fragmentListInfo{};
  _fragmentListInfoBuffer->fetchData(&fragmentListInfo);
  if (fragmentListInfo.voxelFragmentCount == 0) {
    // remove svo buffer allocation rec, so new allocations can be made to this memory region
    auto const &it = _chunkIndexToBufferAllocResult.find(chunkIndex);
    if (it != _chunkIndexToBufferAllocResult.end()) {
      _chunkBufferMemoryAllocator->deallocate(it->second);
      _chunkIndexToBufferAllocResult.erase(chunkIndex);
    }

    // alter the pointer stored in the chunk indices buffer
    uint32_t zero = 0;
    _octreeBufferWriteOffsetBuffer->fillData(&zero);
    cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
    _chunkIndicesBufferUpdaterPipeline->recordCommand(cmdBuffer, 0, 1, 1, 1);
    endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                          _appContext->getGraphicsQueue(), cmdBuffer);

    // delete the image
    auto const &it2 = _chunkIndexToFieldImagesMap.find(chunkIndex);
    if (it2 != _chunkIndexToFieldImagesMap.end()) {
      _chunkIndexToFieldImagesMap.erase(it2);
    }

    _logger->info("chunk {} {} {} is removed because it's empty", chunkIndex.x, chunkIndex.y,
                  chunkIndex.z);

    return;
  }

  // ensure the image has been created before copying
  auto const &it = _chunkIndexToFieldImagesMap.find(chunkIndex);
  if (it == _chunkIndexToFieldImagesMap.end()) {
    _logger->info("creating new image for chunk");
    _chunkIndexToFieldImagesMap[chunkIndex] =
        std::make_unique<Image>(ImageDimensions{_configContainer->terrainInfo->chunkVoxelDim + 1,
                                                _configContainer->terrainInfo->chunkVoxelDim + 1,
                                                _configContainer->terrainInfo->chunkVoxelDim + 1},
                                VK_FORMAT_R16_UINT,
                                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  }

  // save from buffer to image
  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  ImageForwardingPair f{_chunkFieldImage.get(),  _chunkIndexToFieldImagesMap[chunkIndex].get(),
                        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL};
  f.forwardCopy(cmdBuffer);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);

  // octree construction
  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  // wait until the image is ready
  submitInfo.waitSemaphoreCount = 0;
  // signal a semaphore after excecution finished
  submitInfo.signalSemaphoreCount = 0;
  // wait for no stage
  VkPipelineStageFlags waitStages{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
  submitInfo.pWaitDstStageMask = &waitStages;

  std::vector<VkCommandBuffer> commandBuffersToSubmit2{_octreeCreationCommandBuffer};
  submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersToSubmit2.size());
  submitInfo.pCommandBuffers    = commandBuffersToSubmit2.data();

  vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_appContext->getGraphicsQueue());

  uint32_t octreeBufferLength = 0;
  _octreeBufferLengthBuffer->fetchData(&octreeBufferLength);

  // remove allocation
  auto const &it2 = _chunkIndexToBufferAllocResult.find(chunkIndex);
  if (it2 != _chunkIndexToBufferAllocResult.end()) {
    _chunkBufferMemoryAllocator->deallocate(it2->second);
    // erasing is omitted here, since we will overwrite the allocation result
  }
  _chunkIndexToBufferAllocResult[chunkIndex] =
      _chunkBufferMemoryAllocator->allocate(octreeBufferLength * sizeof(uint32_t));
  uint32_t writeOffsetInBytes = _chunkIndexToBufferAllocResult[chunkIndex].offset();

  // print offset in mb
  _logger->info("allocated memory from the memory pool: {} mb",
                static_cast<float>(writeOffsetInBytes) / (1024 * 1024));

  VkBufferCopy bufCopy = {
      0,                                     // srcOffset
      writeOffsetInBytes,                    // dstOffset,
      octreeBufferLength * sizeof(uint32_t), // size
  };

  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  // copy staging buffer to main buffer
  vkCmdCopyBuffer(cmdBuffer, _chunkOctreeBuffer->getVkBuffer(),
                  _appendedOctreeBuffer->getVkBuffer(), 1, &bufCopy);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);

  uint32_t writeOffsetInUint32 = writeOffsetInBytes / sizeof(uint32_t) + 1U;
  _octreeBufferWriteOffsetBuffer->fillData(&writeOffsetInUint32);
  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  // write the chunks image, according to the accumulated buffer offset
  // we should do it here, since we can cull null chunks here after the voxels are decided
  _chunkIndicesBufferUpdaterPipeline->recordCommand(cmdBuffer, 0, 1, 1, 1);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);
}

void SvoBuilder::_buildChunkFromNoise(ChunkIndex chunkIndex) {
  _resetBufferDataForNewChunkGeneration(chunkIndex);

  VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

  // construct field image
  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  _chunkFieldConstructionPipeline->recordCommand(cmdBuffer, 0,
                                                 _configContainer->terrainInfo->chunkVoxelDim + 1,
                                                 _configContainer->terrainInfo->chunkVoxelDim + 1,
                                                 _configContainer->terrainInfo->chunkVoxelDim + 1);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);

  // construct voxels into fragmentlist buffer
  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  _chunkVoxelCreationPipeline->recordCommand(
      cmdBuffer, 0, _configContainer->terrainInfo->chunkVoxelDim,
      _configContainer->terrainInfo->chunkVoxelDim, _configContainer->terrainInfo->chunkVoxelDim);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);

  // intermediate step: check if the fragment list is empty, if so, we can skip the octree creation
  G_FragmentListInfo fragmentListInfo{};
  _fragmentListInfoBuffer->fetchData(&fragmentListInfo);
  if (fragmentListInfo.voxelFragmentCount == 0) {
    return;
  }

  // octree construction
  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  // wait until the image is ready
  submitInfo.waitSemaphoreCount = 0;
  // signal a semaphore after excecution finished
  submitInfo.signalSemaphoreCount = 0;
  // wait for no stage
  VkPipelineStageFlags waitStages{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
  submitInfo.pWaitDstStageMask = &waitStages;

  std::vector<VkCommandBuffer> commandBuffersToSubmit2{_octreeCreationCommandBuffer};
  submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersToSubmit2.size());
  submitInfo.pCommandBuffers    = commandBuffersToSubmit2.data();

  vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_appContext->getGraphicsQueue());

  uint32_t octreeBufferLength = 0;
  _octreeBufferLengthBuffer->fetchData(&octreeBufferLength);

  // remove allocation
  auto const &it = _chunkIndexToBufferAllocResult.find(chunkIndex);
  if (it != _chunkIndexToBufferAllocResult.end()) {
    _chunkBufferMemoryAllocator->deallocate(it->second);
    // erasing is omitted here, since we are going to overwrite it anyway
  }
  _chunkIndexToBufferAllocResult[chunkIndex] =
      _chunkBufferMemoryAllocator->allocate(octreeBufferLength * sizeof(uint32_t));
  uint32_t writeOffsetInBytes = _chunkIndexToBufferAllocResult[chunkIndex].offset();

  // print offset in mb
  _logger->info("allocated memory from the memory pool: {} mb",
                static_cast<float>(writeOffsetInBytes) / (1024 * 1024));

  VkBufferCopy bufCopy = {
      0,                                     // srcOffset
      writeOffsetInBytes,                    // dstOffset,
      octreeBufferLength * sizeof(uint32_t), // size
  };

  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  // copy staging buffer to main buffer
  vkCmdCopyBuffer(cmdBuffer, _chunkOctreeBuffer->getVkBuffer(),
                  _appendedOctreeBuffer->getVkBuffer(), 1, &bufCopy);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);

  uint32_t writeOffsetInUint32 = writeOffsetInBytes / sizeof(uint32_t) + 1U;
  _octreeBufferWriteOffsetBuffer->fillData(&writeOffsetInUint32);
  cmdBuffer = beginSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool());
  // write the chunks image, according to the accumulated buffer offset
  // we should do it here, since we can cull null chunks here after the voxels are decided
  _chunkIndicesBufferUpdaterPipeline->recordCommand(cmdBuffer, 0, 1, 1, 1);
  endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                        _appContext->getGraphicsQueue(), cmdBuffer);
}

void SvoBuilder::_createImages() {
  _chunkFieldImage =
      std::make_unique<Image>(ImageDimensions{_configContainer->terrainInfo->chunkVoxelDim + 1,
                                              _configContainer->terrainInfo->chunkVoxelDim + 1,
                                              _configContainer->terrainInfo->chunkVoxelDim + 1},
                              VK_FORMAT_R16_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}

// voxData is passed in to decide the size of some buffers dureing allocation
void SvoBuilder::_createBuffers(size_t maximumOctreeBufferSize) {
  _chunkIndicesBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t) * _configContainer->terrainInfo->chunksDim.x *
          _configContainer->terrainInfo->chunksDim.y * _configContainer->terrainInfo->chunksDim.z,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _counterBuffer = std::make_unique<Buffer>(sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            MemoryStyle::kDedicated);

  uint32_t sizeInWorstCase =
      std::ceil(static_cast<float>(_configContainer->terrainInfo->chunkVoxelDim *
                                   _configContainer->terrainInfo->chunkVoxelDim *
                                   _configContainer->terrainInfo->chunkVoxelDim) *
                sizeof(uint32_t) * 8.F / 7.F);

  _logger->info("estimated chunk staging buffer size : {} mb",
                static_cast<float>(sizeInWorstCase) / (1024 * 1024));

  _chunkOctreeBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t) * _configContainer->terrainInfo->chunkVoxelDim *
          _configContainer->terrainInfo->chunkVoxelDim *
          _configContainer->terrainInfo->chunkVoxelDim,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      MemoryStyle::kHostVisible);

  _indirectFragLengthBuffer = std::make_unique<Buffer>(sizeof(G_IndirectDispatchInfo),
                                                       VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                       MemoryStyle::kDedicated);

  _appendedOctreeBuffer = std::make_unique<Buffer>(maximumOctreeBufferSize,
                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                   MemoryStyle::kDedicated);

  uint32_t maximumFragmentListBufferSize =
      sizeof(G_FragmentListEntry) * _configContainer->terrainInfo->chunkVoxelDim *
      _configContainer->terrainInfo->chunkVoxelDim * _configContainer->terrainInfo->chunkVoxelDim;
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

  _chunkEditingInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_ChunkEditingInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _octreeBufferLengthBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _octreeBufferWriteOffsetBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);
}

void SvoBuilder::_initBufferData() {
  // clear the chunks buffer
  std::vector<uint32_t> chunksData(_configContainer->terrainInfo->chunksDim.x *
                                       _configContainer->terrainInfo->chunksDim.y *
                                       _configContainer->terrainInfo->chunksDim.z,
                                   0);
  _chunkIndicesBuffer->fillData(chunksData.data());
}

void SvoBuilder::_createDescriptorSetBundle() {
  _descriptorSetBundle =
      std::make_unique<DescriptorSetBundle>(_appContext, 1, VK_SHADER_STAGE_COMPUTE_BIT);

  _descriptorSetBundle->bindStorageImage(0, _chunkFieldImage.get());

  _descriptorSetBundle->bindStorageBuffer(1, _chunkIndicesBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(2, _indirectFragLengthBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(3, _counterBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(4, _chunkOctreeBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(5, _fragmentListBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(6, _octreeBuildInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(7, _indirectAllocNumBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(8, _fragmentListInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(9, _chunksInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(10, _octreeBufferLengthBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(11, _octreeBufferWriteOffsetBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(12, _chunkEditingInfoBuffer.get());

  _descriptorSetBundle->create();
}

void SvoBuilder::_createPipelines() {
  _chunkIndicesBufferUpdaterPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunkIndicesBufferUpdater.comp"),
      WorkGroupSize{8, 8, 8}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _chunkIndicesBufferUpdaterPipeline->compileAndCacheShaderModule(false);
  _chunkIndicesBufferUpdaterPipeline->build();

  _chunkFieldConstructionPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunkFieldConstruction.comp"),
      WorkGroupSize{8, 8, 8}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _chunkFieldConstructionPipeline->compileAndCacheShaderModule(false);
  _chunkFieldConstructionPipeline->build();

  _chunkFieldModificationPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunkFieldModification.comp"),
      WorkGroupSize{8, 8, 8}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _chunkFieldModificationPipeline->compileAndCacheShaderModule(false);
  _chunkFieldModificationPipeline->build();

  _chunkVoxelCreationPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunkVoxelCreation.comp"),
      WorkGroupSize{8, 8, 8}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _chunkVoxelCreationPipeline->compileAndCacheShaderModule(false);
  _chunkVoxelCreationPipeline->build();

  _chunkModifyArgPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("chunkModifyArg.comp"),
      WorkGroupSize{1, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _chunkModifyArgPipeline->compileAndCacheShaderModule(false);
  _chunkModifyArgPipeline->build();

  _initNodePipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("octreeInitNode.comp"),
      WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _initNodePipeline->compileAndCacheShaderModule(false);
  _initNodePipeline->build();

  _tagNodePipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("octreeTagNode.comp"),
      WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _tagNodePipeline->compileAndCacheShaderModule(false);
  _tagNodePipeline->build();

  _allocNodePipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("octreeAllocNode.comp"),
      WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _allocNodePipeline->compileAndCacheShaderModule(false);
  _allocNodePipeline->build();

  _modifyArgPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("octreeModifyArg.comp"),
      WorkGroupSize{1, 1, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _modifyArgPipeline->compileAndCacheShaderModule(false);
  _modifyArgPipeline->build();
}

void SvoBuilder::_recordCommandBuffers() { _recordOctreeCreationCommandBuffer(); }

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
