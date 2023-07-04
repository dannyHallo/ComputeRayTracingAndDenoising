#pragma once

#include <memory>
#include <vector>

#include "utils/vulkan.h"

#include "DrawableModel.h"
#include "render-context/FlatRenderPass.h"
#include "render-context/ForwardRenderPass.h"
#include "render-context/RenderPass.h"

enum class RenderPassType { eFlat, eForward };

class Scene {
public:
  Scene(RenderPassType type);
  void writeRenderCommand(VkCommandBuffer &commandBuffer, const size_t currentFrame);
  void addModel(std::shared_ptr<DrawableModel> model);
  std::shared_ptr<RenderPass> getRenderPass();

private:
  std::vector<std::shared_ptr<DrawableModel>> m_models;
  std::shared_ptr<RenderPass> m_RenderPass;

  void _initFlatRenderPass();
  void _initForwardRenderPass();
};