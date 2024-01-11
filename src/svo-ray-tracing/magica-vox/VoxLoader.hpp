#pragma once

#include <memory>
#include <string>

struct ogt_vox_scene;
class ImData;
class Logger;
namespace VoxLoader {
std::unique_ptr<ImData> loadImg(std::string const &pathToFile, Logger *logger);
}; // namespace VoxLoader