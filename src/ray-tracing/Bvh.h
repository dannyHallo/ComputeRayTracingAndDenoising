#pragma once

#include "../utils/glm.h"
#include "GpuModels.h"
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
const glm::vec3 eps(0.0001f);

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

bool nodeCompare(BvhNode0 &a, BvhNode0 &b);

Aabb surroundingBox(Aabb box0, Aabb box1);

Aabb objectBoundingBox(const GpuModel::Triangle &t);

/// @return the bounding box of all objects
Aabb objectListBoundingBox(std::vector<Object0> &objects);

inline bool boxCompare(const GpuModel::Triangle &a, const GpuModel::Triangle &b,
                       const int axis);

bool boxXCompare(const Object0 &a, const Object0 &b);
bool boxYCompare(const Object0 &a, const Object0 &b);
bool boxZCompare(const Object0 &a, const Object0 &b);

// Since GPU can't deal with tree structures we need to create a flattened BVH.
// Stack is used instead of a tree.
std::vector<GpuModel::BvhNode>
createBvh(const std::vector<Object0> &srcObjects);
} // namespace Bvh
