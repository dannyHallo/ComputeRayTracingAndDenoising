#pragma once

#include "../utils/glm.h"
#include "GpuModels.h"
#include <algorithm>
#include <stack>
#include <stdlib.h> /* srand, rand */
#include <vector>

/**
 * Functions and structs for bvh construction.
 * Since GPU doesn't support recursion, non-recursive structures are used as much as possible.
 */
namespace Bvh {
const glm::vec3 eps(0.0001f);

// Axis-aligned bounding box.
struct Aabb {
  alignas(16) glm::vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
  alignas(16) glm::vec3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

  int longestAxis() {
    float x = abs(max[0] - min[0]);
    float y = abs(max[1] - min[1]);
    float z = abs(max[2] - min[2]);

    if (y > x && y > z)
      return 1;
    if (z > x && z > y)
      return 2;
    return 0;
  }

  static int randomAxis() { return rand() % 3; }
};

// Utility structure to keep track of the initial triangle index in the triangles array while sorting.
struct Object0 {
  uint32_t index;
  GpuModel::Triangle t;
};

// Intermediate BvhNode structure needed for constructing Bvh.
struct BvhNode0 {
  Aabb box;
  // index refers to the index in the final array of nodes. Used for sorting a flattened Bvh.
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

bool nodeCompare(BvhNode0 &a, BvhNode0 &b) { return a.index < b.index; }

Aabb surroundingBox(Aabb box0, Aabb box1) { return {glm::min(box0.min, box1.min), glm::max(box0.max, box1.max)}; }

Aabb objectBoundingBox(const GpuModel::Triangle &t) {
  // Need to add eps to correctly construct an AABB for flat objects like planes.
  return {glm::min(glm::min(t.v0, t.v1), t.v2) - eps, glm::max(glm::max(t.v0, t.v1), t.v2) + eps};
}

/// @return the bounding box of all objects
Aabb objectListBoundingBox(std::vector<Object0> &objects) {
  Aabb tempBox, outputBox;
  bool firstBox = true;

  for (auto &object : objects) {
    tempBox   = objectBoundingBox(object.t);
    outputBox = firstBox ? tempBox : surroundingBox(outputBox, tempBox);
    firstBox  = false;
  }

  return outputBox;
}

inline bool boxCompare(const GpuModel::Triangle &a, const GpuModel::Triangle &b, const int axis) {
  Aabb boxA = objectBoundingBox(a);
  Aabb boxB = objectBoundingBox(b);

  return boxA.min[axis] < boxB.min[axis];
}

bool boxXCompare(const Object0 &a, const Object0 &b) { return boxCompare(a.t, b.t, 0); }

bool boxYCompare(const Object0 &a, const Object0 &b) { return boxCompare(a.t, b.t, 1); }

bool boxZCompare(const Object0 &a, const Object0 &b) { return boxCompare(a.t, b.t, 2); }

// Since GPU can't deal with tree structures we need to create a flattened BVH.
// Stack is used instead of a tree.
std::vector<GpuModel::BvhNode> createBvh(const std::vector<Object0> &srcObjects) {
  std::vector<BvhNode0> intermediate;
  int nodeCounter = 0;
  std::stack<BvhNode0> nodeStack;

  BvhNode0 root;
  root.index   = nodeCounter++;
  root.objects = srcObjects;
  nodeStack.push(root);

  while (!nodeStack.empty()) {
    BvhNode0 currentNode = nodeStack.top();
    nodeStack.pop();

    currentNode.box = objectListBoundingBox(currentNode.objects);

    size_t objectSpan = currentNode.objects.size();

    if (objectSpan <= 1) {
      intermediate.push_back(currentNode);
      continue;
    } else {
      int axis        = currentNode.box.longestAxis();
      auto comparator = (axis == 0) ? boxXCompare : (axis == 1) ? boxYCompare : boxZCompare;
      std::sort(currentNode.objects.begin(), currentNode.objects.end(), comparator);

      auto mid = objectSpan / 2;

      BvhNode0 leftNode;
      leftNode.index = nodeCounter++;
      for (int i = 0; i < mid; i++) {
        leftNode.objects.push_back(currentNode.objects[i]);
      }
      nodeStack.push(leftNode);

      BvhNode0 rightNode;
      rightNode.index = nodeCounter++;
      for (size_t i = mid; i < objectSpan; i++) {
        rightNode.objects.push_back(currentNode.objects[i]);
      }
      nodeStack.push(rightNode);

      currentNode.leftNodeIndex  = leftNode.index;
      currentNode.rightNodeIndex = rightNode.index;
      intermediate.push_back(currentNode);
    }
  }
  std::sort(intermediate.begin(), intermediate.end(), nodeCompare);

  std::vector<GpuModel::BvhNode> output;
  output.reserve(intermediate.size());
  for (int i = 0; i < intermediate.size(); i++) {
    output.push_back(intermediate[i].getGpuModel());
  }
  return output;
}
} // namespace Bvh
