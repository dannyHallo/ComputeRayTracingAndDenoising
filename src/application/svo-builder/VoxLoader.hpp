#pragma once

#include "VoxData.hpp"

#include <string>

struct ogt_vox_scene;
struct ogt_vox_transform;
class Logger;

namespace VoxLoader {
struct VoxTransform {
  VoxTransform(int x, int y, int z) : x(x), y(y), z(z) {}
  VoxTransform(ogt_vox_transform const *transform);

  VoxTransform operator+(VoxTransform const &other) const;
  VoxTransform operator-(VoxTransform const &other) const;

  int x;
  int y;
  int z;
};

VoxData fetchDataFromFile(std::string const &pathToFile);
}; // namespace VoxLoader