#include "TrisScene.hpp"
#include "utils/config/RootDir.h"

namespace GpuModel {

std::vector<Triangle> getTriangles(const std::string &path, uint32_t materialIndex,
                                   glm::vec3 offset, glm::vec3 scale) {
  Mesh mesh(path);

  std::hash<std::string> hasher;
  auto meshId = static_cast<uint32_t>(hasher(path));

  std::vector<Triangle> triangles;
  size_t numTris = mesh.indices.size() / 3;

  for (size_t i = 0; i < numTris; i++) {
    int i0 = mesh.indices[3 * i];
    int i1 = mesh.indices[3 * i + 1];
    int i2 = mesh.indices[3 * i + 2];

    Triangle t{mesh.vertices[i0].pos * scale + offset, mesh.vertices[i1].pos * scale + offset,
               mesh.vertices[i2].pos * scale + offset, materialIndex, meshId};

    triangles.push_back(t);
  }
  return triangles;
}

TrisScene::TrisScene() {
  // struct and classes without default initializer can be inited by this way
  Material gray  = {MaterialType::Lambertian, glm::vec3(1.F, 1.F, 1.F)};
  Material red   = {MaterialType::Lambertian, glm::vec3(.9F, .1F, .1F)};
  Material green = {MaterialType::Lambertian, glm::vec3(.1F, .9F, .1F)};
  Material pink  = {MaterialType::Lambertian, glm::vec3(1.F, .3F, .36F)};
  Material cyan  = {MaterialType::Lambertian, glm::vec3(.3F, .9F, .6F)};

  Material whiteLight = {MaterialType::LightSource, glm::vec3(1.3F, 1.3F, 1.3F)};

  Material metal = {MaterialType::Metal, glm::vec3(1.0F, 1.0F, 1.0F)};
  Material glass = {MaterialType::Glass, glm::vec3(1.0F, 1.0F, 1.0F)};

  materials.push_back(whiteLight);

  materials.push_back(gray);
  materials.push_back(red);
  materials.push_back(green);
  materials.push_back(pink);
  materials.push_back(cyan);
  materials.push_back(metal);

  glm::vec3 cornellBoxScale = {1 / 552.8F, 1 / 548.8F, 1 / 559.2F};
  glm::vec3 cornellBoxPos   = {-0.5F, -0.5F, 1};
  // each getTriangles function loads a series of triangles from a file
  // (vector), which shares the same meshId and matId
  auto floor    = getTriangles(kPathToResourceFolder + "models/tris/cornellbox/floor.obj", 1,
                               cornellBoxPos, cornellBoxScale);
  auto left     = getTriangles(kPathToResourceFolder + "models/tris/cornellbox/left.obj", 4,
                               cornellBoxPos, cornellBoxScale);
  auto right    = getTriangles(kPathToResourceFolder + "models/tris/cornellbox/right.obj", 5,
                               cornellBoxPos, cornellBoxScale);
  auto light    = getTriangles(kPathToResourceFolder + "models/tris/cornellbox/light.obj", 0,
                               cornellBoxPos, cornellBoxScale);
  auto shortbox = getTriangles(kPathToResourceFolder + "models/tris/cornellbox/shortbox.obj", 1,
                               cornellBoxPos, cornellBoxScale);
  auto tallbox  = getTriangles(kPathToResourceFolder + "models/tris/cornellbox/tallbox.obj", 1,
                               cornellBoxPos, cornellBoxScale);
  // then all triangles are added to a single vector
  triangles.insert(triangles.end(), floor.begin(), floor.end());
  triangles.insert(triangles.end(), left.begin(), left.end());
  triangles.insert(triangles.end(), right.begin(), right.end());
  triangles.insert(triangles.end(), light.begin(), light.end());
  triangles.insert(triangles.end(), shortbox.begin(), shortbox.end());
  triangles.insert(triangles.end(), tallbox.begin(), tallbox.end());

  std::vector<Bvh::Object0> objects;

  for (uint32_t i = 0; i < triangles.size(); i++) {
    Triangle t = triangles[i];
    if (materials[t.materialIndex].type == MaterialType::LightSource) {
      // add light separatly
      // calculate the area of this lighting triangle
      float area = glm::length(glm::cross(t.v0, t.v1)) * 0.5F;
      lights.emplace_back(Light{i, area});
      objects.emplace_back(Bvh::Object0{i, t, glm::vec3(-0.2F, 0, 0), glm::vec3(0.2F, 0, 0)});
    } else {
      objects.emplace_back(Bvh::Object0{i, t});
    }
  }

  bvhNodes = Bvh::createBvh(objects);
}
}; // namespace GpuModel