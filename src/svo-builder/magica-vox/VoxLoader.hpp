#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct ogt_vox_scene;
class Logger;

struct VoxData {
  uint32_t voxelResolution;

  // fragmentlist
  std::vector<uint32_t> coordinates;
  std::vector<uint32_t> properties;

  std::array<uint32_t, 256> paletteData; // NOLINT
};

namespace VoxLoader {
VoxData fetchDataFromFile(std::string const &pathToFile);
}; // namespace VoxLoader