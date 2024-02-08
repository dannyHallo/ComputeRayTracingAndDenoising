#include "VoxLoader.hpp"

#define OGT_VOX_IMPLEMENTATION
#include "ogt-tools/ogt_vox.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <vector>

// https://github.com/jpaver/opengametools/blob/master/demo/demo_vox.cpp
namespace VoxLoader {
struct InstanceData {
  uint32_t modelIndex;
  VoxTransform transform;
};

namespace {
// a helper function to load a magica voxel scene given a filename.
ogt_vox_scene const *_loadVoxelScene(std::string const &pathToFile, uint32_t sceneReadFlags = 0) {
  // open the file
  FILE *fp = nullptr;
  if (fopen_s(&fp, pathToFile.c_str(), "rb") != 0) {
    fp = nullptr;
  }
  assert(fp != nullptr && "failed to open vox file, check the path");

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

uint32_t _getVoxelResolution(ogt_vox_scene const *scene,
                             std::vector<InstanceData> const &instanceData) {
  // assert(model->size_x == model->size_y && model->size_y == model->size_z &&
  //        "only cubic models are supported");
  // assert((model->size_x & (model->size_x - 1)) == 0 && "model size must be a power of 2");

  // find min and maxextent of x, y, and z by iterating over all instances
  int minExtentX = std::numeric_limits<int>::max();
  int minExtentY = std::numeric_limits<int>::max();
  int minExtentZ = std::numeric_limits<int>::max();
  int maxExtentX = std::numeric_limits<int>::min();
  int maxExtentY = std::numeric_limits<int>::min();
  int maxExtentZ = std::numeric_limits<int>::min();

  for (auto const &instance : instanceData) {
    auto const *model = scene->models[instance.modelIndex];
    minExtentX        = std::min(minExtentX, instance.transform.x);
    minExtentY        = std::min(minExtentY, instance.transform.y);
    minExtentZ        = std::min(minExtentZ, instance.transform.z);
    maxExtentX = std::max(maxExtentX, instance.transform.x + static_cast<int>(model->size_x));
    maxExtentY = std::max(maxExtentY, instance.transform.y + static_cast<int>(model->size_y));
    maxExtentZ = std::max(maxExtentZ, instance.transform.z + static_cast<int>(model->size_z));
  }

  uint32_t deltaX = maxExtentX - minExtentX;
  uint32_t deltaY = maxExtentY - minExtentY;
  uint32_t deltaZ = maxExtentZ - minExtentZ;

  uint32_t resolution = std::max(deltaX, std::max(deltaY, deltaZ));
  // assert((resolution & (resolution - 1)) == 0 && "resolution must be a power of 2");
  std::cout << "resolution: " << resolution << std::endl;
  // now ceil that to the next power of 2
  resolution = static_cast<uint32_t>(std::pow(2, std::ceil(std::log(resolution) / std::log(2))));

  std::cout << "ceiled resolution: " << resolution << std::endl;

  return resolution;
}

// this example just counts the number of solid voxels in this model, but an importer
// would probably do something like convert the model into a triangle mesh.
VoxData _parseSceneInstances(ogt_vox_scene const *scene,
                             std::vector<InstanceData> const &instanceData) {
  VoxData voxData{};
  voxData.voxelResolution = _getVoxelResolution(scene, instanceData);

  auto const &palette = scene->palette.color;

  // fill palette data
  for (int i = 0; i < 256; i++) {
    auto const &color       = palette[i];
    uint32_t convertedColor = 0;

    // reverse the order of rgba because of packing standard specified here
    // https://registry.khronos.org/OpenGL-Refpages/gl4/html/unpackUnorm.xhtml
    convertedColor |= color.r;
    convertedColor |= color.g << 8;
    convertedColor |= color.b << 16;
    convertedColor |= color.a << 24;
    voxData.paletteData[i] = convertedColor;
  }

  // fill image data
  for (auto const &instance : instanceData) {
    uint32_t voxelIndex = 0;
    auto const *model   = scene->models[instance.modelIndex];

    for (uint32_t z = 0; z < model->size_z; z++) {
      for (uint32_t y = 0; y < model->size_y; y++) {
        for (uint32_t x = 0; x < model->size_x; x++) {
          uint32_t shiftedX = x + instance.transform.x;
          uint32_t shiftedY = y + instance.transform.y;
          uint32_t shiftedZ = z + instance.transform.z;

          // if color index == 0, this voxel is empty, otherwise it is solid.
          uint8_t const colorIndex = model->voxel_data[voxelIndex];
          bool isVoxelValid        = (colorIndex != 0);

          // std::cout << "index: " << voxelIndex << "x: " << shiftedX << " y: " << shiftedY
          //           << " z: " << shiftedZ << " colorIndex: " << static_cast<uint32_t>(colorIndex)
          //           << std::endl;

          if (isVoxelValid) {

            // in this way we can ensure these arrays use the same entry
            // voxData.coordinates.push_back(x);
            // voxData.properties.push_back(static_cast<uint32_t>(colorIndex));
            uint32_t coordinatesData = 0;
            coordinatesData |= shiftedX << 20;
            coordinatesData |= shiftedZ << 10;
            coordinatesData |= shiftedY;

            uint32_t propertiesData = 0;
            // propertiesData |= colorIndex << 16; TODO:

            voxData.fragmentList.emplace_back(coordinatesData, propertiesData);
          }
          voxelIndex++;
        }
      }
    }
  }

  std::cout << "fragment list size: " << voxData.fragmentList.size() << std::endl;

  return voxData;
}

// find the minimum coord among all instances, and shift all instances by that amount
void _shiftInstanceTransforms(std::vector<InstanceData> &instanceData) {
  VoxTransform minTransform{std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
                            std::numeric_limits<int>::max()};
  for (auto const &instance : instanceData) {
    minTransform.x = std::min(minTransform.x, instance.transform.x);
    minTransform.y = std::min(minTransform.y, instance.transform.y);
    minTransform.z = std::min(minTransform.z, instance.transform.z);
  }

  for (auto &instance : instanceData) {
    instance.transform = instance.transform - minTransform;
  }
}
} // namespace

VoxTransform::VoxTransform(ogt_vox_transform const *transform)
    : x(static_cast<int>(transform->m30)), y(static_cast<int>(transform->m31)),
      z(static_cast<int>(transform->m32)) {}

VoxTransform VoxTransform::operator+(VoxTransform const &other) const {
  return VoxTransform{VoxTransform{x + other.x, y + other.y, z + other.z}};
}

VoxTransform VoxTransform::operator-(VoxTransform const &other) const {
  return VoxTransform{VoxTransform{x - other.x, y - other.y, z - other.z}};
}

VoxData fetchDataFromFile(std::string const &pathToFile) {
  const ogt_vox_scene *scene = _loadVoxelScene(pathToFile);

  std::vector<InstanceData> instanceData{};
  for (size_t instanceIndex = 0; instanceIndex < scene->num_instances; instanceIndex++) {
    auto const instance  = scene->instances[instanceIndex];
    auto const transform = instance.transform;
    VoxTransform trans{&transform};
    instanceData.emplace_back(InstanceData{instance.model_index, trans});
  }

  _shiftInstanceTransforms(instanceData);

  VoxData voxData = _parseSceneInstances(scene, instanceData);

  return voxData;
}
} // namespace VoxLoader