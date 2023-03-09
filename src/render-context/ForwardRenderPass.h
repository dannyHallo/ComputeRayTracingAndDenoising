#pragma once

#include "memory/Image.h"
#include "utils/vulkan.h"

#include "RenderPass.h"
#include <iostream>
#include <memory>
#include <vector>

class ForwardRenderPass : public RenderPass {
public:
  std::shared_ptr<VkRenderPass> getBody() override;

  std::shared_ptr<VkFramebuffer> getFramebuffer(size_t index) override;

  ForwardRenderPass();

  ~ForwardRenderPass();

  std::shared_ptr<Image> getColorImage() override;
  std::shared_ptr<Image> getDepthImage();

private:
  std::shared_ptr<Image> m_colorImage;
  std::shared_ptr<Image> m_depthImage;
  std::shared_ptr<VkRenderPass> m_renderPass;
  std::shared_ptr<VkFramebuffer> m_framebuffer;

  void createRenderPass();

  void createFramebuffers();

  VkFormat findDepthFormat();

  bool hasStencilComponent(VkFormat format);

  void createDepthResources();

  void createColorResources();
};