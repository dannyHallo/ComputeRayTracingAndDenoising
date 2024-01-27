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

std::vector<uint32_t> _createByHand() {
  std::vector<uint32_t> voxelBuffer{0x80000008, 0x80000008, 0x80000008, 0x80000008,
                                    0x80000008, 0x80000008, 0x80000008, 0x80000008,
                                    0xC0000024, 0xC0000024, 0xC0000024, 0xC0000024,
                                    0xC0000024, 0xC0000024, 0x00000000, 0x00000000};
  return voxelBuffer;
}

std::vector<std::string> const kFileNames = {"4_test",    "32_chr_knight", "64_monu1", "128_monu2",
                                             "128_monu3", "128_monu4",     "256_monu4"};

VoxData _fetchVoxData(size_t index) {
  std::string const kPathToVoxFile =
      kPathToResourceFolder + "models/vox/" + kFileNames[index] + ".vox";
  return VoxLoader::fetchDataFromFile(kPathToVoxFile);
}
} // namespace

SvoBuilder::SvoBuilder(VulkanApplicationContext *appContext, Logger *logger)
    : _appContext(appContext), _logger(logger) {}

SvoBuilder::~SvoBuilder() = default;

void SvoBuilder::init() {
  // data preparation
  VoxData voxData = _fetchVoxData(0);

  // buffers
  _createBuffers(voxData);
  _initBufferData(voxData);

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();
}

// voxData is passed in to decide the size of some buffers dureing allocation
void SvoBuilder::_createBuffers(const VoxData &voxData) {
  _paletteBuffer = std::make_unique<Buffer>(sizeof(uint32_t) * voxData.paletteData.size(),
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            MemoryAccessingStyle::kCpuToGpuOnce);

  _atomicCounterBuffer =
      std::make_unique<Buffer>(sizeof(G_AtomicCounter), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               MemoryAccessingStyle::kCpuToGpuOnce);

  _octreeBuffer = std::make_unique<Buffer>(sizeof(uint32_t) * 100000,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // TODO: size
                                           MemoryAccessingStyle::kCpuToGpuOnce);

  _fragmentListBuffer = std::make_unique<Buffer>(
      sizeof(G_FragmentListEntry) * voxData.fragmentList.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      MemoryAccessingStyle::kCpuToGpuOnce);

  _buildInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_BuildInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryAccessingStyle::kCpuToGpuOnce);

  _indirectBuffer = std::make_unique<Buffer>(sizeof(G_IndirectData),
                                             VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                             MemoryAccessingStyle::kCpuToGpuOnce);
}

void SvoBuilder::_initBufferData(VoxData const &voxData) {
  _paletteBuffer->fillData(voxData.paletteData.data());
  _fragmentListBuffer->fillData(voxData.fragmentList.data());
  _octreeBuffer->fillData(_createByHand().data()); // TODO: delete this temp function
}

// TODO:
void SvoBuilder::_createDescriptorSetBundle() {
  _descriptorSetBundle =
      std::make_unique<DescriptorSetBundle>(_appContext, _logger, 1, VK_SHADER_STAGE_COMPUTE_BIT);

  _descriptorSetBundle->bindStorageBuffer(0, _atomicCounterBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(1, _octreeBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(2, _fragmentListBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(3, _buildInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(4, _indirectBuffer.get());
  // the palette buffer is left for the svo tracer to use, but its data is obtained here

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