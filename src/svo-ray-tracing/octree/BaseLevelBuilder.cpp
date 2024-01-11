#include "BaseLevelBuilder.hpp"

#include <cmath>
#include <vector>

namespace BaseLevelBuilder {

uint32_t constexpr kAllEight = 0x0000FFFF;
uint32_t constexpr kOnlyOne  = 0x00000101;

namespace {
void _buildVoxel(ImageData *imageData, ImCoor3D const &coor, uint32_t writeVal) {
  imageData->imageStore(coor, writeVal);
}

void _buildPlane(ImageData *imageData, ImCoor3D const &imageSize, int y, uint32_t writeVal) {
  for (int x = 0; x < imageSize.x; ++x) {
    for (int z = 0; z < imageSize.z; ++z) {
      _buildVoxel(imageData, {x, y, z}, writeVal);
    }
  }
}

void _buildTheWorstCase(ImageData *imageData, ImCoor3D const &imageSize) {
  for (int y = 0; y < imageSize.y; ++y) {
    _buildPlane(imageData, imageSize, y, kOnlyOne);
  }
}

void _buildANormalScene(ImageData *imageData, ImCoor3D const &imageSize) {
  for (int y = 0; y < static_cast<int>(static_cast<float>(imageSize.y) / 4.F); ++y) {
    _buildPlane(imageData, imageSize, y, kAllEight);
  }
}
} // namespace

const std::vector<ImCoor3D> kBuildVoxelCoors{{0, 0, 0}};

void build(ImageData *imageData, ImCoor3D const &imageSize) {
  _buildTheWorstCase(imageData, imageSize);
  // _buildANormalScene(imageData, imageSize);

  // for (auto const &coor : kBuildVoxelCoors) {
  //   _buildVoxel(imageData, coor);
  // }
}

} // namespace BaseLevelBuilder
