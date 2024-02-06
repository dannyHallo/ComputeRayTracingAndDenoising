#include "SvoBuilder.hpp"

#include "SvoBuilderDataGpu.hpp"
#include "memory/Buffer.hpp"
#include "pipelines/ComputePipeline.hpp"
#include "pipelines/DescriptorSetBundle.hpp"
#include "svo-builder/VoxLoader.hpp"
#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip> // for advanced cout formatting
#include <iostream>
#include <stack>

namespace {

VoxData _fetchVoxData() {
  std::string const kFileName      = "128_monu2_light";
  std::string const kPathToVoxFile = kPathToResourceFolder + "models/vox/" + kFileName + ".vox";
  return VoxLoader::fetchDataFromFile(kPathToVoxFile);
}

// the worst case: for every fragment, we assume they all reside in different parents, in the
// current building algorithm, a non-leaf node's children must be initialized (to zero if empty at
// last), however, when the estimated amount of nodes exceeds the actual amount, the size of the
// octree is fixed.
// https://eisenwave.github.io/voxel-compression-docs/svo/svo.html

uint32_t _getMaximumNodeCountOfOctree(VoxData const &voxData) {
  uint32_t maximumSize = 0;

  uint32_t res                     = voxData.voxelResolution;
  uint32_t elementCount            = voxData.fragmentList.size();
  uint32_t maximumNodeCountAtLevel = 0;

  while (res != 1) {
    maximumNodeCountAtLevel = std::min(elementCount * 8, res * res * res);
    maximumSize += maximumNodeCountAtLevel;
    res          = res >> 1;
    elementCount = std::ceil(static_cast<float>(elementCount) / 8);
  }

  return maximumSize;
}
} // namespace

SvoBuilder::SvoBuilder(VulkanApplicationContext *appContext, Logger *logger)
    : _appContext(appContext), _logger(logger) {}

SvoBuilder::~SvoBuilder() {
  vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1, &_commandBuffer);
}

void SvoBuilder::init() {
  // data preparation
  VoxData voxData = _fetchVoxData();

  // buffers
  _createBuffers(voxData);
  _initBufferData(voxData);
  uint32_t octreeVoxelResolution = voxData.voxelResolution;
  _voxelLevelCount               = static_cast<uint32_t>(std::log2(octreeVoxelResolution));
  uint32_t voxelFragmentCount    = voxData.fragmentList.size();
  _logger->print("voxel resolution: {}", octreeVoxelResolution);
  _logger->print("voxel level count: {}", _voxelLevelCount);
  _logger->print("voxel fragment count: {}", voxelFragmentCount);

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();
  _recordCommandBuffer(voxelFragmentCount, _voxelLevelCount);
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

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &_commandBuffer;

  vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo, svoBuildingDoneFence);
}

// voxData is passed in to decide the size of some buffers dureing allocation
void SvoBuilder::_createBuffers(const VoxData &voxData) {
  _paletteBuffer = std::make_unique<Buffer>(sizeof(uint32_t) * voxData.paletteData.size(),
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            MemoryAccessingStyle::kCpuToGpuOnce);

  _atomicCounterBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryAccessingStyle::kCpuToGpuOnce);

  uint32_t estimatedOctreeBufferSize = sizeof(uint32_t) * _getMaximumNodeCountOfOctree(voxData);
  _logger->print("estimated octree buffer size: {} mb",
                 static_cast<float>(estimatedOctreeBufferSize) / (1024 * 1024));

  _octreeBuffer =
      std::make_unique<Buffer>(estimatedOctreeBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               MemoryAccessingStyle::kGpuOnly);

  _fragmentListBuffer = std::make_unique<Buffer>(
      sizeof(G_FragmentListEntry) * voxData.fragmentList.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      MemoryAccessingStyle::kCpuToGpuOnce);

  _buildInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_BuildInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryAccessingStyle::kCpuToGpuOnce);

  _indirectDispatchInfoBuffer = std::make_unique<Buffer>(sizeof(G_IndirectDispatchInfo),
                                                         VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                         MemoryAccessingStyle::kCpuToGpuOnce);

  _fragmentListInfoBuffer =
      std::make_unique<Buffer>(sizeof(G_FragmentListInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               MemoryAccessingStyle::kCpuToGpuOnce);
}

void SvoBuilder::_initBufferData(VoxData const &voxData) {
  _paletteBuffer->fillData(voxData.paletteData.data());

  uint32_t atomicCounterInitData = 1;
  _atomicCounterBuffer->fillData(&atomicCounterInitData);

  // octree buffer is left uninitialized

  _fragmentListBuffer->fillData(voxData.fragmentList.data());

  G_BuildInfo buildInfo{};
  buildInfo.allocBegin = 0;
  buildInfo.allocNum   = 8;
  _buildInfoBuffer->fillData(&buildInfo);

  G_IndirectDispatchInfo indirectDispatchInfo{};
  indirectDispatchInfo.dispatchX = 1;
  indirectDispatchInfo.dispatchY = 1;
  indirectDispatchInfo.dispatchZ = 1;
  _indirectDispatchInfoBuffer->fillData(&indirectDispatchInfo);

  G_FragmentListInfo fragmentListInfo{};
  fragmentListInfo.voxelResolution    = voxData.voxelResolution;
  fragmentListInfo.voxelFragmentCount = static_cast<uint32_t>(voxData.fragmentList.size());
  _fragmentListInfoBuffer->fillData(&fragmentListInfo);
}

void SvoBuilder::_createDescriptorSetBundle() {
  _descriptorSetBundle =
      std::make_unique<DescriptorSetBundle>(_appContext, 1, VK_SHADER_STAGE_COMPUTE_BIT);

  // the palette buffer is left for the svo tracer to use
  _descriptorSetBundle->bindStorageBuffer(0, _atomicCounterBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(1, _octreeBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(2, _fragmentListBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(3, _buildInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(4, _indirectDispatchInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(5, _fragmentListInfoBuffer.get());

  _descriptorSetBundle->create();
}

void SvoBuilder::_createPipelines() {
  // indirect dispatch
  {
    std::vector<uint32_t> shaderCode{
#include "octreeInitNode.spv"
    };
    _initNodePipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get());
    _initNodePipeline->init();
  }
  {
    std::vector<uint32_t> shaderCode{
#include "octreeTagNode.spv"
    };
    _tagNodePipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get());
    _tagNodePipeline->init();
  }
  // indirect dispatch
  {
    std::vector<uint32_t> shaderCode{
#include "octreeAllocNode.spv"
    };
    _allocNodePipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{64, 1, 1}, _descriptorSetBundle.get());
    _allocNodePipeline->init();
  }
  {
    std::vector<uint32_t> shaderCode{
#include "octreeModifyArg.spv"
    };
    _modifyArgPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{1, 1, 1}, _descriptorSetBundle.get());
    _modifyArgPipeline->init();
  }
}

void SvoBuilder::_recordCommandBuffer(uint32_t voxelFragmentCount, uint32_t octreeLevelCount) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = _appContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkResult result = vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, &_commandBuffer);
  assert(result == VK_SUCCESS && "vkAllocateCommandBuffers failed");

  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(_commandBuffer, &beginInfo);

  // create the standard memory barrier
  VkMemoryBarrier memoryBarrier1 = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  memoryBarrier1.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier1.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  VkMemoryBarrier memoryBarrier2 = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  memoryBarrier2.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier2.dstAccessMask =
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  for (uint32_t level = 0; level < octreeLevelCount; level++) {
    _initNodePipeline->recordIndirectCommand(_commandBuffer, 0,
                                             _indirectDispatchInfoBuffer->getVkBuffer());
    vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier1, 0, nullptr, 0,
                         nullptr);

    _tagNodePipeline->recordCommand(_commandBuffer, 0, voxelFragmentCount, 1, 1);

    if (level != octreeLevelCount - 1) {
      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier1, 0, nullptr,
                           0, nullptr);

      _allocNodePipeline->recordIndirectCommand(_commandBuffer, 0,
                                                _indirectDispatchInfoBuffer->getVkBuffer());
      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier1, 0, nullptr,
                           0, nullptr);

      _modifyArgPipeline->recordCommand(_commandBuffer, 0, 1, 1, 1);

      // be cautious! the indirect buffer is modified in the modifyArg pipeline, so we need to
      // make sure it is prepared for the next indirect dispatch too
      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 1, &memoryBarrier2, 0, nullptr, 0, nullptr);
    }
  }

  vkEndCommandBuffer(_commandBuffer);
}