#pragma once

#include "ImCoor.hpp"

#include <cstdint>
#include <map>

class ImageData {
public:
  ImageData(const ImCoor3D &imageSize) : _imageSize(imageSize){};
  ~ImageData() = default;

  // default copy and move
  ImageData(const ImageData &)            = default;
  ImageData(ImageData &&)                 = default;
  ImageData &operator=(const ImageData &) = default;
  ImageData &operator=(ImageData &&)      = default;

  [[nodiscard]] ImCoor3D getImageSize() const { return _imageSize; }
  [[nodiscard]] std::map<ImCoor3D, uint32_t> const &getImageData() const { return _imageData; }

  void imageStore(const ImCoor3D &coor, uint32_t val);
  [[nodiscard]] uint32_t imageLoad(const ImCoor3D &coor) const;

private:
  ImCoor3D _imageSize;
  std::map<ImCoor3D, uint32_t> _imageData;

  [[nodiscard]] bool _isInBound(const ImCoor3D &coor) const;
};
