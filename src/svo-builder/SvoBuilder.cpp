#include "SvoBuilder.hpp"

#include "SvoBuilderDataGpu.hpp"
#include "memory/Buffer.hpp"
#include "memory/Image.hpp"
#include "pipelines/ComputePipeline.hpp"
#include "pipelines/DescriptorSetBundle.hpp"
#include "svo-builder/VoxLoader.hpp"
#include "utils/config/RootDir.h"
#include "utils/file-io/ShaderFileReader.hpp"
#include "utils/logger/Logger.hpp"

#include <chrono>
#include <cmath>
#include <iomanip> // for advanced cout formatting
#include <iostream>
#include <stack>

static uint32_t constexpr kChunkSize = 256;

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
} // namespace

SvoBuilder::SvoBuilder(VulkanApplicationContext *appContext, Logger *logger)
    : _appContext(appContext), _logger(logger) {}

SvoBuilder::~SvoBuilder() {
  vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1, &_commandBuffer);
}

void SvoBuilder::init() {
  _voxelLevelCount = static_cast<uint32_t>(std::log2(kChunkSize));

  // images
  _createImages();

  // buffers
  _createBuffers();
  _initBufferData();

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();
  _recordCommandBuffer();
}

void SvoBuilder::build(VkFence svoBuildingDoneFence) {
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

  vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo, svoBuildingDoneFence);
}

void SvoBuilder::_createImages() {
  _chunkFieldImage = std::make_unique<Image>(kChunkSize + 1, kChunkSize + 1, kChunkSize + 1,
                                             VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);
}

// voxData is passed in to decide the size of some buffers dureing allocation
void SvoBuilder::_createBuffers() {
  _counterBuffer = std::make_unique<Buffer>(sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            MemoryAccessingStyle::kCpuToGpuOnce);

  // uint32_t estimatedOctreeBufferSize = sizeof(uint32_t) * _getMaximumNodeCountOfOctree(voxData);

  uint32_t maximumOctreeBufferSize = std::ceil(static_cast<float>(sizeof(uint32_t)) * kChunkSize *
                                               kChunkSize * kChunkSize * 8.F / 7.F);
  _logger->info("octree buffer size: {} mb",
                static_cast<float>(maximumOctreeBufferSize) / (1024 * 1024));

  _indirectFragLengthBuffer = std::make_unique<Buffer>(sizeof(G_IndirectDispatchInfo),
                                                       VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                       MemoryAccessingStyle::kGpuOnly);

  _octreeBuffer = std::make_unique<Buffer>(
      maximumOctreeBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryAccessingStyle::kGpuOnly);

  uint32_t maximumFragmentListBufferSize =
      sizeof(G_FragmentListEntry) * kChunkSize * kChunkSize * kChunkSize;
  _fragmentListBuffer =
      std::make_unique<Buffer>(maximumFragmentListBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               MemoryAccessingStyle::kGpuOnly);

  _logger->info("fragment list buffer size: {} mb",
                static_cast<float>(maximumFragmentListBufferSize) / (1024 * 1024));

  _buildInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_BuildInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryAccessingStyle::kCpuToGpuOnce);

  _indirectAllocNumBuffer = std::make_unique<Buffer>(sizeof(G_IndirectDispatchInfo),
                                                     VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                     MemoryAccessingStyle::kCpuToGpuOnce);

  _fragmentListInfoBuffer =
      std::make_unique<Buffer>(sizeof(G_FragmentListInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               MemoryAccessingStyle::kCpuToGpuOnce);
}

void SvoBuilder::_initBufferData() {
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
  fragmentListInfo.voxelResolution    = kChunkSize;
  fragmentListInfo.voxelFragmentCount = 0;
  _fragmentListInfoBuffer->fillData(&fragmentListInfo);
}

void SvoBuilder::_createDescriptorSetBundle() {
  _descriptorSetBundle =
      std::make_unique<DescriptorSetBundle>(_appContext, 1, VK_SHADER_STAGE_COMPUTE_BIT);

  _descriptorSetBundle->bindStorageImage(6, _chunkFieldImage.get());
  _descriptorSetBundle->bindStorageBuffer(7, _indirectFragLengthBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(0, _counterBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(1, _octreeBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(2, _fragmentListBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(3, _buildInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(4, _indirectAllocNumBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(5, _fragmentListInfoBuffer.get());

  _descriptorSetBundle->create();
}

void SvoBuilder::_createPipelines() {
  {
    auto shaderCode = ShaderFileReader::readShaderFile(KPathToCompiledShadersFolder +
                                                       "chunkFieldConstruction.spv");
    _chunkFieldConstructionPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{8, 8, 8}, _descriptorSetBundle.get());
    _chunkFieldConstructionPipeline->init();
  }
  {
    auto shaderCode =
        ShaderFileReader::readShaderFile(KPathToCompiledShadersFolder + "chunkVoxelCreation.spv");
    _chunkVoxelCreationPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{8, 8, 8}, _descriptorSetBundle.get());
    _chunkVoxelCreationPipeline->init();
  }
  {
    auto shaderCode =
        ShaderFileReader::readShaderFile(KPathToCompiledShadersFolder + "chunkModifyArg.spv");
    _chunkModifyArgPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{1, 1, 1}, _descriptorSetBundle.get());
    _chunkModifyArgPipeline->init();
  }
  {
    auto shaderCode =
        ShaderFileReader::readShaderFile(KPathToCompiledShadersFolder + "octreeInitNode.spv");
    _initNodePipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get());
    _initNodePipeline->init();
  }
  {
    auto shaderCode =
        ShaderFileReader::readShaderFile(KPathToCompiledShadersFolder + "octreeTagNode.spv");
    _tagNodePipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get());
    _tagNodePipeline->init();
  }
  {
    auto shaderCode =
        ShaderFileReader::readShaderFile(KPathToCompiledShadersFolder + "octreeAllocNode.spv");
    _allocNodePipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get());
    _allocNodePipeline->init();
  }
  {
    auto shaderCode =
        ShaderFileReader::readShaderFile(KPathToCompiledShadersFolder + "octreeModifyArg.spv");
    _modifyArgPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{1, 1, 1}, _descriptorSetBundle.get());
    _modifyArgPipeline->init();
  }
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

  _chunkFieldConstructionPipeline->recordCommand(_commandBuffer, 0, kChunkSize + 1, kChunkSize + 1,
                                                 kChunkSize + 1);

  vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &shaderAccessBarrier, 0, nullptr,
                       0, nullptr);

  _chunkVoxelCreationPipeline->recordCommand(_commandBuffer, 0, kChunkSize, kChunkSize, kChunkSize);

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

      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 1, &indirectReadBarrier, 0, nullptr, 0, nullptr);
    }
  }

  vkEndCommandBuffer(_commandBuffer);
}