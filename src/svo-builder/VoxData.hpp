#pragma once

#include "SvoBuilderDataGpu.hpp"

#include <array>
#include <cstdint>
#include <vector>

struct VoxData {
  uint32_t voxelResolution;
  std::vector<G_FragmentListEntry> fragmentList;
  std::array<uint32_t, 256> paletteData;
};
