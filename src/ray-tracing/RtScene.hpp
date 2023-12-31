#pragma once

#include "Bvh.hpp"
#include "GpuModels.hpp"
#include "scene/Mesh.hpp"
#include "utils/config/RootDir.h"
#include "utils/incl/Glm.hpp"

#include <functional> // for std::hash
#include <iostream>
#include <vector>

glm::vec3 multiplyAccordingly(glm::vec3 v1, glm::vec3 v2);

namespace GpuModel {
std::vector<Triangle> getTriangles(const std::string &path, uint materialIndex,
                                   glm::vec3 offset = {0, 0, 0}, glm::vec3 scale = {1, 1, 1});

/*
 * The scene to be ray traced. All objects are split into triangles and put into
 * a common triangle array.
 */
struct Scene {
  // triangles contain all triangles from all objects in the scene.
  std::vector<Triangle> triangles;
  std::vector<Material> materials;
  std::vector<Light> lights;
  std::vector<BvhNode> bvhNodes;

  Scene();
};
} // namespace GpuModel
