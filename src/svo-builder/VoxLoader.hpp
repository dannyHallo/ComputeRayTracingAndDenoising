#pragma once

#include "VoxData.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct ogt_vox_scene;
class Logger;

namespace VoxLoader {
VoxData fetchDataFromFile(std::string const &pathToFile);
}; // namespace VoxLoader