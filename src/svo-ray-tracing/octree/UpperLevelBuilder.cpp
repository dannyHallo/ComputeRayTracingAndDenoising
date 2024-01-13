#include "UpperLevelBuilder.hpp"

#include <cassert>

namespace UpperLevelBuilder {
namespace {

uint32_t _offsetId(int x, int y, int z) {
  uint32_t constexpr kXWeight = 4;
  uint32_t constexpr kYWeight = 2;
  uint32_t constexpr kZWeight = 1;
  return x * kXWeight + y * kYWeight + z * kZWeight;
}

uint32_t _combine8To1(ImData const *_lowerLevelData, ImCoor3D const &coorCur) {
  uint32_t constexpr shiftingMask = 0x00000001;

  uint32_t dataWrite = 0;
  for (int x = 0; x < 2; x++) {
    for (int y = 0; y < 2; y++) {
      for (int z = 0; z < 2; z++) {
        ImCoor3D coorLowerLevel = {coorCur.x * 2 + x, coorCur.y * 2 + y, coorCur.z * 2 + z};
        uint32_t dataRead       = _lowerLevelData->imageLoad(coorLowerLevel);

        if (dataRead != 0U) {
          dataWrite |= shiftingMask << _offsetId(x, y, z);
        }
      }
    }
  }
  return dataWrite;
}
} // namespace

void build(ImData const *lowerLevelData, ImData *thisLevelData) {
  ImCoor3D coorCur{0, 0, 0};

  ImCoor3D _lowerLevelImageSize = lowerLevelData->getImageSize();

  for (int x = 0; x < _lowerLevelImageSize.x / 2; x++) {
    coorCur.x = x;
    for (int y = 0; y < _lowerLevelImageSize.y / 2; y++) {
      coorCur.y = y;
      for (int z = 0; z < _lowerLevelImageSize.z / 2; z++) {
        coorCur.z = z;

        uint32_t dataWrite = _combine8To1(lowerLevelData, coorCur);
        // if dataWrite is 0, we don't need to store it, because it means all leaf nodes are 0
        if (dataWrite != 0U) {
          thisLevelData->imageStore(coorCur, dataWrite);
        }
      }
    }
  }
}

} // namespace UpperLevelBuilder
