#pragma once

#include <vector>

#include "Material.h"
#include "Mesh.h"
#include "memory/Buffer.h"

class DrawableModel {
public:
  DrawableModel(std::shared_ptr<Material> material, std::string modelPath);

  DrawableModel(std::shared_ptr<Material> material, MeshType type);

  std::shared_ptr<Material> getMaterial();
  void drawCommand(VkCommandBuffer &commandBuffer, size_t currentFrame);

private:
  std::shared_ptr<Material> m_material;
  Buffer mVertexBuffer;
  Buffer mIndexBuffer;
  uint32_t mNumIndices;

  // allocate vertex buffer and fill it with data
  void initVertexBuffer(const Mesh &mesh);

  // allocate index buffer and fill it with data
  void initIndexBuffer(const Mesh &mesh);
};