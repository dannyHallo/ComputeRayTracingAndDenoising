#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct ogt_vox_scene;
class ImData;
class Logger;

struct VoxData {
  std::unique_ptr<ImData> imageData;
  std::vector<uint32_t> paletteData;
};

namespace VoxLoader {
VoxData fetchDataFromFile(std::string const &pathToFile, Logger *logger);
}; // namespace VoxLoader