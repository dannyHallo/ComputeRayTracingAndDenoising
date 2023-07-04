#pragma once

#include <memory>

#include "memory/Image.h"
#include "utils/vulkan.h"

class RenderPass {
public:
  virtual std::shared_ptr<VkRenderPass> getBody()                     = 0;
  virtual std::shared_ptr<VkFramebuffer> getFramebuffer(size_t index) = 0;
  virtual std::shared_ptr<Image> getColorImage()                      = 0;
};