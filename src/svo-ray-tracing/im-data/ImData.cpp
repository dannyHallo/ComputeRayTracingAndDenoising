#include "ImData.hpp"

#include <cassert>
#include <iostream>

void ImageData::imageStore(ImCoor3D const &coor, uint32_t val) {
  assert(_isInBound(coor) && "ImageData::imageStore: out of bound");
  _imageData[coor] = val;
}

uint32_t ImageData::imageLoad(ImCoor3D const &coor) const {
  assert(_isInBound(coor) && "ImageData::imageLoad: out of bound");

  // if key exists, reture val, if not, return 0
  auto it = _imageData.find(coor);
  if (it != _imageData.end()) {
    return it->second;
  }
  return 0;
}

bool ImageData::_isInBound(ImCoor3D const &coor) const {
  if (coor.x < 0 || coor.x >= _imageSize.x) {
    return false;
  }
  if (coor.y < 0 || coor.y >= _imageSize.y) {
    return false;
  }
  if (coor.z < 0 || coor.z >= _imageSize.z) {
    return false;
  }
  return true;
}