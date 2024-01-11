#include "VoxLoader.hpp"

#define OGT_VOX_IMPLEMENTATION
// disable all warnings from ogt_vox (gcc & clang)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include "ogt-tools/ogt_vox.h"
#pragma GCC diagnostic pop

#include "svo-ray-tracing/im-data/ImCoor.hpp"
#include "svo-ray-tracing/im-data/ImData.hpp"
#include "utils/logger/Logger.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <iostream>

// Ref: https://github.com/jpaver/opengametools/blob/master/demo/demo_vox.cpp

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

  assert(scene != nullptr && "Failed to load vox scene");
  return scene;
}

ImCoor3D _getModelSize(ogt_vox_model const *model) {
  return ImCoor3D(static_cast<int>(model->size_x), static_cast<int>(model->size_y),
                  static_cast<int>(model->size_z));
}

// this example just counts the number of solid voxels in this model, but an importer
// would probably do something like convert the model into a triangle mesh.
void _parseModel(ImData *imageData, ogt_vox_scene const *scene, ogt_vox_model const *model,
                 Logger *logger) {
  auto const &palette = scene->palette.color; // TODO: use color lookup later

  uint32_t voxelIndex = 0;
  for (int z = 0; z < model->size_z; z++) {
    for (int y = 0; y < model->size_y; y++) {
      for (int x = 0; x < model->size_x; x++, voxelIndex++) {
        // if color index == 0, this voxel is empty, otherwise it is solid.
        uint32_t const colorIndex = model->voxel_data[voxelIndex];
        bool isVoxelValid         = (colorIndex != 0);
        if (isVoxelValid) {
          // std::cout << "Color: " << static_cast<int>(palette[colorIndex].r) << " "
          //           << static_cast<int>(palette[colorIndex].g) << " "
          //           << static_cast<int>(palette[colorIndex].b) << std::endl;

          // TODO: to be changed later to increase efficiency
          imageData->imageStore(ImCoor3D(x, y, z), 0x0000FFFF);
        }
      }
    }
  }
}

} // namespace

std::unique_ptr<ImData> loadImg(std::string const &pathToFile, Logger *logger) {
  const ogt_vox_scene *scene = _loadVoxelScene(pathToFile);

  // iterate over all models and print basic information about the model.
  assert(scene->num_models == 1 && "Only one model is supported");
  size_t constexpr modelIndex = 0;

  auto imData = std::make_unique<ImData>(_getModelSize(scene->models[modelIndex]));

  const ogt_vox_model *model = scene->models[modelIndex];

  _parseModel(imData.get(), scene, model, logger);

  ogt_vox_destroy_scene(scene);

  return std::move(imData);
}
} // namespace VoxLoader