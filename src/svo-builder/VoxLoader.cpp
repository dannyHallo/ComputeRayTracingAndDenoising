#include "VoxLoader.hpp"

#define OGT_VOX_IMPLEMENTATION
// disable all warnings from ogt_vox (gcc & clang)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include "ogt-tools/ogt_vox.h"
#pragma GCC diagnostic pop

#include "utils/logger/Logger.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <vector>

// https://github.com/jpaver/opengametools/blob/master/demo/demo_vox.cpp

namespace VoxLoader {
namespace {
// a helper function to load a magica voxel scene given a filename.
ogt_vox_scene const *_loadVoxelScene(std::string const &pathToFile, uint32_t sceneReadFlags = 0) {
  // open the file
  FILE *fp = nullptr;
  if (fopen_s(&fp, pathToFile.c_str(), "rb") != 0) {
    fp = nullptr;
  }
  assert(fp != nullptr && "Failed to open vox file");

  // get the buffer size which matches the size of the file
  fseek(fp, 0, SEEK_END);
  uint32_t const bufferSize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // load the file into a memory buffer
  std::vector<uint8_t> voxBuffer(bufferSize);
  fread(voxBuffer.data(), bufferSize, 1, fp);
  fclose(fp);

  // construct the scene from the buffer
  ogt_vox_scene const *scene =
      ogt_vox_read_scene_with_flags(voxBuffer.data(), bufferSize, sceneReadFlags);

  assert(scene != nullptr && "failed to load vox scene");
  return scene;
}

uint32_t _getVoxelResolution(ogt_vox_model const *model) {
  assert(model->size_x == model->size_y && model->size_y == model->size_z &&
         "only cubic models are supported");
  assert((model->size_x & (model->size_x - 1)) == 0 && "model size must be a power of 2");
  return model->size_x;
}

// #include <iostream>

// this example just counts the number of solid voxels in this model, but an importer
// would probably do something like convert the model into a triangle mesh.
VoxData _parseModel(ogt_vox_scene const *scene, ogt_vox_model const *model) {
  VoxData voxData{};
  voxData.voxelResolution = _getVoxelResolution(model);

  auto const &palette = scene->palette.color;

  // fill palette
  for (int i = 0; i < 256; i++) {
    auto const &color       = palette[i];
    uint32_t convertedColor = 0;
    convertedColor |= static_cast<uint32_t>(color.r) << 24;
    convertedColor |= static_cast<uint32_t>(color.g) << 16;
    convertedColor |= static_cast<uint32_t>(color.b) << 8;
    convertedColor |= static_cast<uint32_t>(color.a);
    voxData.paletteData[i] = convertedColor;
  }

  // fill image data
  uint32_t voxelIndex = 0;
  for (int y = 0; y < model->size_y; y++) {
    for (int x = 0; x < model->size_x; x++) {
      for (int z = 0; z < model->size_z; z++) {
        // if color index == 0, this voxel is empty, otherwise it is solid.
        uint8_t const colorIndex = model->voxel_data[voxelIndex];
        bool isVoxelValid        = (colorIndex != 0);

        if (isVoxelValid) {
          // in this way we can ensure these arrays use the same entry
          // voxData.coordinates.push_back(x);
          // voxData.properties.push_back(static_cast<uint32_t>(colorIndex));
          uint32_t coordinatesData = 0;
          coordinatesData |= static_cast<uint32_t>(x) << 20;
          coordinatesData |= static_cast<uint32_t>(y) << 10;
          coordinatesData |= static_cast<uint32_t>(z);

          uint32_t propertiesData = 0;
          propertiesData |= static_cast<uint32_t>(colorIndex);

          voxData.fragmentList.emplace_back(G_FragmentListEntry{coordinatesData, propertiesData});

          //   std::cout << "coor: " << std::setw(3) << x << " " << std::setw(3) << y << " "
          //             << std::setw(3) << z;
          //   std::cout << " prop: " << std::setw(3) << static_cast<uint32_t>(colorIndex) <<
          //   std::endl;
        }
        voxelIndex++;
      }
    }
  }
  return voxData;
}

} // namespace

VoxData fetchDataFromFile(std::string const &pathToFile) {
  const ogt_vox_scene *scene = _loadVoxelScene(pathToFile);

  // obtain model
  assert(scene->num_models == 1 && "Only one model is supported");
  size_t constexpr modelIndex = 0;
  const ogt_vox_model *model  = scene->models[modelIndex];

  VoxData voxData = _parseModel(scene, model);

  ogt_vox_destroy_scene(scene);

  return voxData;
}
} // namespace VoxLoader