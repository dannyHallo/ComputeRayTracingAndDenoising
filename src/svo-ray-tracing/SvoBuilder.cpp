#include "SvoBuilder.hpp"

#include "svo-ray-tracing/magica-vox/VoxLoader.hpp"
#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <cassert>
#include <chrono>
#include <iomanip> // for advanced cout formatting
#include <iostream>
#include <stack>

namespace {
void _logBuffer(const std::vector<uint32_t> &vec) {
  for (size_t i = 0; i < vec.size(); ++i) {
    std::cout << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << vec[i]
              << "u";

    if (i != vec.size() - 1) {
      std::cout << ", ";
    }

    if ((i + 1) % 8 == 0) {
      std::cout << std::endl;
    }
  }
  std::cout << std::endl;
}

template <class T> void _printBufferSizeInMb(std::vector<T> const &voxelBuffer, Logger *logger) {
  // print buffer size in mb
  uint32_t constexpr kBytesInMb = 1024 * 1024;
  float const kBufferSizeInMb   = static_cast<float>(voxelBuffer.size() * sizeof(T)) / kBytesInMb;
  logger->print("buffer size: {} mb", kBufferSizeInMb);
}

std::vector<uint32_t> _createByHand() {
  std::vector<uint32_t> voxelBuffer{0x80000008, 0x80000008, 0x80000008, 0x80000008,  // NOLINT
                                    0x80000008, 0x80000008, 0x80000008, 0x80000008,  // NOLINT
                                    0xC0000024, 0xC0000024, 0xC0000024, 0xC0000024,  // NOLINT
                                    0xC0000024, 0xC0000024, 0x00000000, 0x00000000}; // NOLINT
  return voxelBuffer;
}
} // namespace

static std::vector<std::string> const kFileNames = {
    "4_test", "32_chr_knight", "64_monu1", "128_monu2", "128_monu3", "128_monu4", "256_monu4"};

SvoBuilder::SvoBuilder(Logger *logger) : _logger(logger) {
  _fetchRawVoxelData(0);
  _createVoxelBuffer();
  _logBuffer(_voxelBuffer);
  _logger->print("svo built!");
}

SvoBuilder::~SvoBuilder() = default;

void SvoBuilder::_fetchRawVoxelData(size_t index) {
  std::string const kPathToVoxFile =
      kPathToResourceFolder + "models/vox/" + kFileNames[index] + ".vox";
  _voxData       = std::make_unique<VoxData>(VoxLoader::fetchDataFromFile(kPathToVoxFile));
  _paletteBuffer = _voxData->paletteData;
}

void SvoBuilder::_createVoxelBuffer() {
  _voxelBuffer = _createByHand();
  _printBufferSizeInMb(_voxelBuffer, _logger);
}
