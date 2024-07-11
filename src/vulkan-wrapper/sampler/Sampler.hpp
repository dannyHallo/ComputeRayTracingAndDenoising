#pragma once

#include "volk.h"

class Sampler {
public:
  enum class AddressMode {
    kRepeat = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    kClamp  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
  };

  Sampler(AddressMode addressMode);
  ~Sampler();

  // disable move and copy
  Sampler(const Sampler &)            = delete;
  Sampler &operator=(const Sampler &) = delete;
  Sampler(Sampler &&)                 = delete;
  Sampler &operator=(Sampler &&)      = delete;

  inline VkSampler &getVkSampler() { return _vkSampler; }

private:
  VkSampler _vkSampler = VK_NULL_HANDLE;
};