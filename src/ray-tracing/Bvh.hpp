#pragma once

#include "../utils/Glm.hpp"
#include "GpuModels.hpp"
#include <algorithm>
#include <stack>
#include <stdlib.h> /* srand, rand */
#include <vector>

/**
 * Functions and structs for bvh construction.
 * Since GPU doesn't support recursion, non-recursive structures are used as
 * much as possible.
 */
namespace Bvh {
const glm::vec3 eps(1e-4F);

struct Aabb {
  alignas(16) glm::vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
  alignas(16) glm::vec3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

  int longestAxis() {
    float x = abs(max[0] - min[0]);
    float y = abs(max[1] - min[1]);
    float z = abs(max[2] - min[2]);

    if (y > x && y > z) return 1;
    if (z > x && z > y) return 2;
    return 0;
  }

  static int randomAxis() { return rand() % 3; }
};

struct Object0 {
  uint32_t index;
  GpuModel::Triangle t;
  glm::vec3 minOffset;
  glm::vec3 maxOffset;

  Object0(uint32_t i, GpuModel::Triangle t)
      : index(i), t(t), minOffset(0), maxOffset(0) {}

  Object0(uint32_t i, GpuModel::Triangle t, glm::vec3 minOffset,
          glm::vec3 maxOffset)
      : index(i), t(t), minOffset(minOffset), maxOffset(maxOffset) {}
};

struct BvhNode0 {
  Aabb box;
  // index refers to the index in the final array of nodes. Used for sorting a
  // flattened Bvh.
  int index          = -1;
  int leftNodeIndex  = -1;
  int rightNodeIndex = -1;
  std::vector<Object0> objects;

  GpuModel::BvhNode getGpuModel() const {
    GpuModel::BvhNode node{};
    node.min            = box.min;
    node.max            = box.max;
    node.leftNodeIndex  = leftNodeIndex;
    node.rightNodeIndex = rightNodeIndex;

    if (leftNodeIndex == -1 && rightNodeIndex == -1) // if leaf node
      node.objectIndex = objects[0].index;

    return node;
  }
};

// Since GPU can't deal with tree structures we need to create a flattened BVH.
// Stack is used instead of a tree.
std::vector<GpuModel::BvhNode>
createBvh(const std::vector<Object0> &srcObjects);
} // namespace Bvh
