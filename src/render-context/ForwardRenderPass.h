#pragma once

#include <memory>

#include "RenderPass.h"
#include "memory/Image.h"

class ForwardRenderPass : public RenderPass {
  std::shared_ptr<Image> m_colorImage;
  std::shared_ptr<Image> m_depthImage;
  std::shared_ptr<VkRenderPass> m_renderPass;
  std::shared_ptr<VkFramebuffer> m_framebuffer;

public:
  ForwardRenderPass();

  ~ForwardRenderPass();

  std::shared_ptr<VkRenderPass> getBody() override;

  std::shared_ptr<VkFramebuffer> getFramebuffer(size_t index) override;

  std::shared_ptr<Image> getColorImage() override;

  std::shared_ptr<Image> getDepthImage();

private:
  void createRenderPass();

  void createFramebuffers();

  VkFormat findDepthFormat();

  bool hasStencilComponent(VkFormat format);

  void createDepthResources();

  void createColorResources();
};