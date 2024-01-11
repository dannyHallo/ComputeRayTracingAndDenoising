#include "BaseLevelBuilder.hpp"

#include <cmath>
#include <vector>

namespace BaseLevelBuilder {

uint32_t constexpr kAllEight = 0x0000FFFF;
uint32_t constexpr kOnlyOne  = 0x00000101;

namespace {
void _buildVoxel(ImData *imageData, ImCoor3D const &coor, uint32_t writeVal) {
  imageData->imageStore(coor, writeVal);
}

void _buildPlane(ImData *imageData, int y, uint32_t writeVal) {
  ImCoor3D const imageSize = imageData->getImageSize();
  for (int x = 0; x < imageSize.x; ++x) {
    for (int z = 0; z < imageSize.z; ++z) {
      _buildVoxel(imageData, {x, y, z}, writeVal);
    }
  }
}

void _buildTheWorstCase(ImData *imageData) {
  ImCoor3D const imageSize = imageData->getImageSize();
  for (int y = 0; y < imageSize.y; ++y) {
    _buildPlane(imageData, y, kOnlyOne);
  }
}

void _buildANormalScene(ImData *imageData) {
  ImCoor3D const imageSize = imageData->getImageSize();
  for (int y = 0; y < static_cast<int>(static_cast<float>(imageSize.y) / 4.F); ++y) {
    _buildPlane(imageData, y, kAllEight);
  }
}

} // namespace

const std::vector<ImCoor3D> kBuildVoxelCoors{{0, 0, 0}};

void build(ImData *imageData) {
  // _buildTheWorstCase(imageData);
  _buildANormalScene(imageData);

  // for (auto const &coor : kBuildVoxelCoors) {
  //   _buildVoxel(imageData, coor);
  // }
}

} // namespace BaseLevelBuilder
