#include "RtScene.h"

glm::vec3 multiplyAccordingly(glm::vec3 v1, glm::vec3 v2) {
  glm::vec3 vOut{};
  vOut.x = v1.x * v2.x;
  vOut.y = v1.y * v2.y;
  vOut.z = v1.z * v2.z;

  return vOut;
}

namespace GpuModel {
std::vector<Triangle> getTriangles(const std::string &path, uint materialIndex,
                                   glm::vec3 offset, glm::vec3 scale) {
  Mesh mesh(path);

  std::hash<std::string> hasher;
  uint32_t meshId = static_cast<uint32_t>(hasher(path));

  std::vector<Triangle> triangles;
  size_t numTris = mesh.indices.size() / 3;

  for (size_t i = 0; i < numTris; i++) {
    int i0 = mesh.indices[3 * i];
    int i1 = mesh.indices[3 * i + 1];
    int i2 = mesh.indices[3 * i + 2];

    Triangle t{multiplyAccordingly(mesh.vertices[i0].pos, scale) + offset,
               multiplyAccordingly(mesh.vertices[i1].pos, scale) + offset,
               multiplyAccordingly(mesh.vertices[i2].pos, scale) + offset,
               materialIndex, meshId};
    triangles.push_back(t);
  }
  return triangles;
}

Scene::Scene() {
  const std::string path_prefix = std::string(ROOT_DIR) + "resources/";

  // struct and classes without default initializer can be inited by this way
  Material gray  = {MaterialType::Lambertian, glm::vec3(1.f, 1.f, 1.f)};
  Material red   = {MaterialType::Lambertian, glm::vec3(.9f, .1f, .1f)};
  Material green = {MaterialType::Lambertian, glm::vec3(.1f, .9f, .1f)};
  Material pink  = {MaterialType::Lambertian, glm::vec3(1.f, .3f, .36f)};
  Material cyan  = {MaterialType::Lambertian, glm::vec3(.3f, .9f, .6f)};

  Material whiteLight = {MaterialType::LightSource,
                         glm::vec3(1.3f, 1.3f, 1.3f)};

  Material metal = {MaterialType::Metal, glm::vec3(1.0f, 1.0f, 1.0f)};
  Material glass = {MaterialType::Glass, glm::vec3(1.0f, 1.0f, 1.0f)};

  materials.push_back(whiteLight);

  materials.push_back(gray);
  materials.push_back(red);
  materials.push_back(green);
  materials.push_back(pink);
  materials.push_back(cyan);
  materials.push_back(metal);

  glm::vec3 cornellBoxScale = {1 / 552.8f, 1 / 548.8f, 1 / 559.2f};
  glm::vec3 cornellBoxPos   = {-0.5f, -0.5f, 1};
  // each getTriangles function loads a series of triangles from a file
  // (vector), which shares the same meshId and matId
  auto floor    = getTriangles(path_prefix + "models/cornellbox/floor.obj", 1,
                               cornellBoxPos, cornellBoxScale);
  auto left     = getTriangles(path_prefix + "models/cornellbox/left.obj", 4,
                               cornellBoxPos, cornellBoxScale);
  auto right    = getTriangles(path_prefix + "models/cornellbox/right.obj", 5,
                               cornellBoxPos, cornellBoxScale);
  auto light    = getTriangles(path_prefix + "models/cornellbox/light.obj", 0,
                               cornellBoxPos, cornellBoxScale);
  auto shortbox = getTriangles(path_prefix + "models/cornellbox/shortbox.obj",
                               1, cornellBoxPos, cornellBoxScale);
  auto tallbox  = getTriangles(path_prefix + "models/cornellbox/tallbox.obj", 1,
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
      float area = glm::length(glm::cross(t.v0, t.v1)) * 0.5f;
      lights.emplace_back(Light{i, area});
      objects.emplace_back(
          Bvh::Object0{i, t, glm::vec3(-0.2F, 0, 0), glm::vec3(0.2F, 0, 0)});
    } else {
      objects.emplace_back(Bvh::Object0{i, t});
    }
  }

  bvhNodes = Bvh::createBvh(objects);
}
}; // namespace GpuModel