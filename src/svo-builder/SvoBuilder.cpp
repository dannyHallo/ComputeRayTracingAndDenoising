#include "SvoBuilder.hpp"

#include "SvoBuilderBufferStructs.hpp"
#include "memory/Buffer.hpp"
#include "pipelines/ComputePipeline.hpp"
#include "pipelines/DescriptorSetBundle.hpp"
#include "svo-builder/VoxLoader.hpp"
#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <cassert>
#include <chrono>
#include <iomanip> // for advanced cout formatting
#include <iostream>
#include <stack>

namespace {
// void _logBuffer(const std::vector<uint32_t> &vec) {
//   for (size_t i = 0; i < vec.size(); ++i) {
//     std::cout << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex <<
//     vec[i]
//               << "u";

//     if (i != vec.size() - 1) {
//       std::cout << ", ";
//     }

//     if ((i + 1) % 8 == 0) {
//       std::cout << std::endl;
//     }
//   }
//   std::cout << std::endl;
// }

// template <class T> void _printBufferSizeInMb(std::vector<T> const &voxelBuffer, Logger *logger) {
//   // print buffer size in mb
//   uint32_t constexpr kBytesInMb = 1024 * 1024;
//   float const kBufferSizeInMb   = static_cast<float>(voxelBuffer.size() * sizeof(T)) /
//   kBytesInMb; logger->print("buffer size: {} mb", kBufferSizeInMb);
// }

// std::vector<uint32_t> _createByHand() {
//   std::vector<uint32_t> voxelBuffer{0x80000008, 0x80000008, 0x80000008, 0x80000008,
//                                     0x80000008, 0x80000008, 0x80000008, 0x80000008,
//                                     0xC0000024, 0xC0000024, 0xC0000024, 0xC0000024,
//                                     0xC0000024, 0xC0000024, 0x00000000, 0x00000000};
//   return voxelBuffer;
// }

// 4_test
// 8_chr_knight
// 16_chr_knight
// 16_box
// 32_chr_knight
// 64_monu1
// 128_box
// 128_monu2
// 128_monu3
// 128_monu4
// 256_monu4

VoxData _fetchVoxData() {
  std::string constexpr kFileName = "256_monu4";

  std::string const kPathToVoxFile = kPathToResourceFolder + "models/vox/" + kFileName + ".vox";
  return VoxLoader::fetchDataFromFile(kPathToVoxFile);
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
  auto octreeVoxelLevel          = static_cast<uint32_t>(std::log2(octreeVoxelResolution));
  uint32_t voxelFragmentCount    = voxData.fragmentList.size();
  _logger->print("octree voxel resolution: {}", octreeVoxelResolution);
  _logger->print("octree voxel level: {}", octreeVoxelLevel);
  _logger->print("voxel fragment count: {}", voxelFragmentCount);

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();
  _recordCommandBuffer(voxelFragmentCount, octreeVoxelLevel);
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

  uint32_t megabytes = 1024 * 1024;
  _octreeBuffer =
      std::make_unique<Buffer>(100 * megabytes,
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // TODO: adaptive size
                               MemoryAccessingStyle::kGpuOnly);

  _fragmentListBuffer = std::make_unique<Buffer>(
      sizeof(G_FragmentListEntry) * voxData.fragmentList.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      MemoryAccessingStyle::kCpuToGpuOnce);

  _buildInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_BuildInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryAccessingStyle::kCpuToGpuOnce);

  _indirectBuffer = std::make_unique<Buffer>(sizeof(G_IndirectData),
                                             VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                             MemoryAccessingStyle::kCpuToGpuOnce);

  _fragmentDataBuffer =
      std::make_unique<Buffer>(sizeof(G_FragmentListData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               MemoryAccessingStyle::kCpuToGpuOnce);
}

void SvoBuilder::_initBufferData(VoxData const &voxData) {
  _paletteBuffer->fillData(voxData.paletteData.data());

  uint32_t atomicCounterInitData = 0;
  _atomicCounterBuffer->fillData(&atomicCounterInitData);

  // octree buffer is left uninitialized
  // _octreeBuffer->fillData(_createByHand().data());

  _fragmentListBuffer->fillData(voxData.fragmentList.data());

  G_BuildInfo buildInfoInitData{};
  buildInfoInitData.allocBegin = 0;
  buildInfoInitData.allocNum   = 8;
  _buildInfoBuffer->fillData(&buildInfoInitData);

  G_IndirectData indirectDataInitData{};
  indirectDataInitData.dispatchX = 1;
  indirectDataInitData.dispatchY = 1;
  indirectDataInitData.dispatchZ = 1;
  _indirectBuffer->fillData(&indirectDataInitData);

  G_FragmentListData fragmentListDataInitData{};
  fragmentListDataInitData.voxelResolution    = voxData.voxelResolution;
  fragmentListDataInitData.voxelFragmentCount = static_cast<uint32_t>(voxData.fragmentList.size());
  _fragmentDataBuffer->fillData(&fragmentListDataInitData);
}

void SvoBuilder::_createDescriptorSetBundle() {
  _descriptorSetBundle =
      std::make_unique<DescriptorSetBundle>(_appContext, _logger, 1, VK_SHADER_STAGE_COMPUTE_BIT);

  // the palette buffer is left for the svo tracer to use
  _descriptorSetBundle->bindStorageBuffer(0, _atomicCounterBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(1, _octreeBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(2, _fragmentListBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(3, _buildInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(4, _indirectBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(5, _fragmentDataBuffer.get());

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
  VkMemoryBarrier memoryBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  memoryBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  auto octreeMemoryBarrier = _octreeBuffer->getMemoryBarrier(
      VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

  auto indirectMemoryBarrier = _indirectBuffer->getMemoryBarrier(
      VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

  auto buildInfoMemoryBarrier =
      _buildInfoBuffer->getMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

  for (uint32_t level = 0; level < octreeLevelCount; level++) {
    // _postProcessingPipeline->recordCommand(cmdBuffer, 0, w, h, 1);
    _initNodePipeline->recordIndirectCommand(_commandBuffer, 0, _indirectBuffer->getVkBuffer());
    vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                         &octreeMemoryBarrier, 0, nullptr);

    _tagNodePipeline->recordCommand(_commandBuffer, 0, voxelFragmentCount, 1, 1);

    if (level != octreeLevelCount - 1) {
      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                           &octreeMemoryBarrier, 0, nullptr);

      _allocNodePipeline->recordIndirectCommand(_commandBuffer, 0, _indirectBuffer->getVkBuffer());
      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr,
                           0, nullptr);

      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                           &octreeMemoryBarrier, 0, nullptr);

      _modifyArgPipeline->recordCommand(_commandBuffer, 0, 1, 1, 1);

      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 0, nullptr, 1, &indirectMemoryBarrier, 0, nullptr);

      vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                           &buildInfoMemoryBarrier, 0, nullptr);
    }
  }

  vkEndCommandBuffer(_commandBuffer);
}