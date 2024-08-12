#pragma once

#include "volk.h"

class VulkanApplicationContext;
class Sampler {
public:
  // see the difference at: https://learnopengl.com/Getting-started/Textures
  enum class AddressMode {
    kRepeat         = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    kMirroredRepeat = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    kClampToEdge    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    kClampToBorder  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
  };

  struct Settings {
    AddressMode addressModeU;
    AddressMode addressModeV;
    AddressMode addressModeW;
  };

  Sampler(VulkanApplicationContext *appContext, Settings const &settings);
  ~Sampler();

  // disable move and copy
  Sampler(const Sampler &)            = delete;
  Sampler &operator=(const Sampler &) = delete;
  Sampler(Sampler &&)                 = delete;
  Sampler &operator=(Sampler &&)      = delete;

  inline VkSampler &getVkSampler() { return _vkSampler; }

private:
  VulkanApplicationContext *_appContext;

  VkSampler _vkSampler = VK_NULL_HANDLE;
};