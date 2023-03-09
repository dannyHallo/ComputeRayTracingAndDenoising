#pragma once

#include "Material.h"
#include "Mesh.h"
#include "memory/Buffer.h"
#include "utils/vulkan.h"

#include <vector>

class DrawableModel {
public:
  DrawableModel(std::shared_ptr<Material> material, std::string modelPath);

  DrawableModel(std::shared_ptr<Material> material, MeshType type);

  std::shared_ptr<Material> getMaterial();
  void drawCommand(VkCommandBuffer &commandBuffer, size_t currentFrame);

private:
  std::shared_ptr<Material> m_material;
  Buffer m_vertexBuffer;
  Buffer m_indexBuffer;
  uint32_t m_numIndices;

  void initVertexBuffer(const Mesh &mesh);

  void initIndexBuffer(const Mesh &mesh);
};