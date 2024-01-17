#include "SvoScene.hpp"

#include "svo-ray-tracing/im-data/ImCoor.hpp"
#include "svo-ray-tracing/magica-vox/VoxLoader.hpp"
#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <cassert>
#include <chrono>
#include <iomanip> // for advanced cout formatting
#include <iostream>
#include <stack>

namespace {

uint32_t constexpr kNextNodePtrOffset = 9;
uint32_t constexpr kNextIsLeafMask    = 0x00000100;
uint32_t constexpr kHasChildMask      = 0x000000FF;

// the best method for counting digit 1s in a 32 bit uint32_t in parallel
// https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
uint32_t bitCount(uint32_t v) {
  const std::vector<uint32_t> S{1U, 2U, 4U, 8U, 16U}; // Magic Binary Numbers
  const std::vector<uint32_t> B{0x55555555U, 0x33333333U, 0x0F0F0F0FU, 0x00FF00FFU, 0x0000FFFFU};

  uint32_t c = v - ((v >> 1) & B[0]);
  c          = ((c >> S[1]) & B[1]) + (c & B[1]);
  c          = ((c >> S[2]) + c) & B[2];
  c          = ((c >> S[3]) + c) & B[3];
  c          = ((c >> S[4]) + c) & B[4];
  return c;
}

// https://stackoverflow.com/questions/1053582/how-does-this-bitwise-operation-check-for-a-power-of-2
bool _imageSizeCanBeDividedByTwo(ImCoor3D const &imageSize) {
  return (imageSize.x & (imageSize.x - 1)) == 0 && (imageSize.y & (imageSize.y - 1)) == 0 &&
         (imageSize.z & (imageSize.z - 1)) == 0;
}

void _printHexFormat(const std::vector<uint32_t> &vec) {
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

bool _checkNextNodeIdxValidaty(uint32_t nextNodeIdx) {
  uint32_t constexpr kAllOnes = 0xFFFFFFFF;

  uint32_t mask = kAllOnes >> kNextNodePtrOffset;
  return (nextNodeIdx & mask) == nextNodeIdx;
}

template <class T> void _printBufferSizeInMb(std::vector<T> const &voxelBuffer, Logger *logger) {
  // print buffer size in mb
  uint32_t constexpr kBytesInMb = 1024 * 1024;
  float const kBufferSizeInMb   = static_cast<float>(voxelBuffer.size() * sizeof(T)) / kBytesInMb;
  logger->print("buffer size: {} mb", kBufferSizeInMb);
}

std::vector<ImCoor3D> _finerLevelCoordinates(ImCoor3D const &fromCourseCoor) {
  std::vector<ImCoor3D> finerCoors{};
  for (int x = 0; x < 2; x++) {
    for (int y = 0; y < 2; y++) {
      for (int z = 0; z < 2; z++) {
        finerCoors.push_back(fromCourseCoor * 2 + ImCoor3D{x, y, z});
      }
    }
  }
  return finerCoors;
}

int exceedingCount = 0;
std::vector<uint32_t> _createByBfs(std::vector<std::unique_ptr<ImData>> const &imageDatas) {
  std::vector<uint32_t> voxelBuffer{};
  int imIdx = static_cast<int>(imageDatas.size() - 1); // start with the root image

  // process the root node
  auto const &imageData = imageDatas[imIdx];

  ImCoor3D const coor{0, 0, 0};
  uint32_t const &data = imageData->imageLoad(coor);
  if (data == 0U) {
    return voxelBuffer;
  }

  std::vector<ImCoor3D> activeCoorsPrev{};
  activeCoorsPrev.push_back(coor);

  uint32_t nextNodeIdx = 1;
  // change the data by filling the first 16 bits with the next bit offset
  uint32_t dataToUse = data | (nextNodeIdx << kNextNodePtrOffset);
  voxelBuffer.push_back(dataToUse);
  nextNodeIdx += bitCount(data & kHasChildMask); // accum the bit offset

  while (--imIdx >= 0) {
    auto const &imageData = imageDatas[imIdx];
    std::vector<ImCoor3D> thisTimeActiveCoors{};
    for (auto const activeCoor : activeCoorsPrev) {
      auto const &finerLevelCoordinates = _finerLevelCoordinates(activeCoor);
      for (auto const &coor : finerLevelCoordinates) {
        uint32_t data = imageData->imageLoad(coor);

        if (data == 0U) {
          continue;
        }

        thisTimeActiveCoors.push_back(coor);

        // image layer with index 0 is the leaf layer, so we don't need to store the next node
        if (imIdx != 0) {
          // assert(_checkNextNodeIdxValidaty(nextNodeIdx) && "nextNodeIdxPtr is too large");
          data |= nextNodeIdx << kNextNodePtrOffset;
          if (!_checkNextNodeIdxValidaty(nextNodeIdx)) {
            exceedingCount++;
          }
          nextNodeIdx += bitCount(data & kHasChildMask); // accum the bit offset
        }

        // image layer with index 1 is the last-to-leaf layer
        if (imIdx == 1) {
          data |= kNextIsLeafMask;
        }
        voxelBuffer.push_back(data);
      }
    }
    activeCoorsPrev = thisTimeActiveCoors;
  }
  return voxelBuffer;
}

struct StackContext {
  uint32_t bufferIndex;
  uint32_t imageIndex;
  ImCoor3D coor;
};

void _visitNode(std::stack<StackContext> &stack, std::vector<uint32_t> &voxelBuffer,
                uint32_t &nextNodeIdx, std::vector<std::unique_ptr<ImData>> const &imageDatas) {
  auto const &[bufferIdx, imIdx, coor] = stack.top();
  stack.pop();

  // change next pointer
  uint32_t &bufferData = voxelBuffer[bufferIdx];
  bufferData |= nextNodeIdx << kNextNodePtrOffset;
  if (!_checkNextNodeIdxValidaty(nextNodeIdx)) {
    if (exceedingCount < 10) {
      std::cout << "nextNodeIdx: " << nextNodeIdx << std::endl;
    }
    exceedingCount++;
  }

  if (imIdx == 1) {
    bufferData |= kNextIsLeafMask;
  }

  auto const &fCoors = _finerLevelCoordinates(coor);
  uint32_t fImDix    = imIdx - 1;
  for (auto const &fCoor : fCoors) {
    auto const &imageData = imageDatas[fImDix];
    uint32_t const data   = imageData->imageLoad(fCoor);
    if (data == 0U) {
      continue;
    }

    voxelBuffer.push_back(data);
    nextNodeIdx++;

    // leaf nodes don't need to be visited
    if (fImDix != 0) {
      stack.emplace(voxelBuffer.size() - 1, fImDix, fCoor);
    }
  }
}

std::vector<uint32_t> _createByDfs(std::vector<std::unique_ptr<ImData>> const &imageDatas) {
  std::vector<uint32_t> voxelBuffer{};

  // add the root node manually
  auto const &imageData = imageDatas.back();
  ImCoor3D const coor{0, 0, 0};
  uint32_t const data = imageData->imageLoad(coor);
  if (data == 0U) {
    // if root node is empty
    return voxelBuffer;
  }
  voxelBuffer.push_back(data);

  std::stack<StackContext> stack{};
  stack.emplace(0, imageDatas.size() - 1, ImCoor3D{0, 0, 0});

  uint32_t nextNodeIdx = 1;
  while (!stack.empty()) {
    _visitNode(stack, voxelBuffer, nextNodeIdx, imageDatas);
  }
  return voxelBuffer;
}
} // namespace

SvoScene::SvoScene(Logger *logger) : _logger(logger) { _run(); }

void SvoScene::_run() {
  _buildImageDatas();
  // _printImageDatas();
  _createVoxelBuffer();
  // _printBuffer();
  _logger->print("SvoScene::_run() done!");
}

void SvoScene::_buildImageDatas() {
  std::vector<std::string> const kFileNames = {
      "4_test", "32_chr_knight", "64_monu1", "128_monu2", "128_monu3", "128_monu4", "256_monu4"};

  // record time
  auto const start = std::chrono::high_resolution_clock::now();

  std::string const kPathToVoxFile = kPathToResourceFolder + "models/vox/" + kFileNames[4] + ".vox";
  auto voxData                     = VoxLoader::fetchDataFromFile(kPathToVoxFile, _logger);

  // print elapse using _logger
  auto const end    = std::chrono::high_resolution_clock::now();
  auto const elapse = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  _logger->print("base image built using {} ms", elapse);

  auto &imageData = voxData.imageData;
  _paletteBuffer  = voxData.paletteData;

  assert(_imageSizeCanBeDividedByTwo(imageData->getImageSize()) &&
         "The base image size should be a power of 2");

  _imageDatas.push_back(std::move(imageData));

  int imIdx = 0;

  ImCoor3D const kRootImageSize = {1, 1, 1};
  while (_imageDatas[imIdx]->getImageSize() != kRootImageSize) {
    auto newImageData = std::make_unique<ImData>(_imageDatas[imIdx]->getImageSize() / 2);
    _imageDatas.push_back(std::move(newImageData));

    // record time
    auto const start = std::chrono::high_resolution_clock::now();
    UpperLevelBuilder::build(_imageDatas[imIdx].get(), _imageDatas[imIdx + 1].get());
    // print elapse using _logger
    auto const end    = std::chrono::high_resolution_clock::now();
    auto const elapse = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    _logger->print("image {} built using {} ms", imIdx + 1, elapse);

    imIdx++;
  }
}

void SvoScene::_printImageDatas() {
  for (auto const &imageData : _imageDatas) {
    std::cout << "imageData->getImageSize(): " << imageData->getImageSize().x << " "
              << imageData->getImageSize().y << " " << imageData->getImageSize().z << std::endl;

    for (auto const &[coor, data] : imageData->getImageData()) {
      std::cout << "coor: " << coor.x << " " << coor.y << " " << coor.z << std::endl;
      std::cout << "data: " << std::hex << data << std::endl;
    }
  }
}

void SvoScene::_createVoxelBuffer() {
  // record time
  auto const start = std::chrono::high_resolution_clock::now();

  // _voxelBuffer = _createByBfs(_imageDatas);
  _voxelBuffer = _createByDfs(_imageDatas);

  // print elapse using _logger
  auto const end    = std::chrono::high_resolution_clock::now();
  auto const elapse = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  _logger->print("voxel buffer created using {} ms", elapse);

  _logger->print("exceedingCount: {}", exceedingCount);

  _printBufferSizeInMb(_voxelBuffer, _logger);
}

void SvoScene::_printBuffer() {
  std::cout << "the buffer is: \n" << std::endl;
  _printHexFormat(_voxelBuffer);
}
