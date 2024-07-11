#include "Sampler.hpp"

#include "app-context/VulkanApplicationContext.hpp"

Sampler::Sampler(Sampler::AddressMode addressMode) {

  VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.magFilter               = VK_FILTER_LINEAR; // For bilinear interpolation
  samplerInfo.minFilter               = VK_FILTER_LINEAR; // For bilinear interpolation
  samplerInfo.anisotropyEnable        = VK_FALSE;
  samplerInfo.maxAnisotropy           = 1;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0F;
  samplerInfo.minLod                  = 0.0F;
  samplerInfo.maxLod                  = 0.0F;

  VkSamplerAddressMode const targetAddrMode = static_cast<VkSamplerAddressMode>(addressMode);

  samplerInfo.addressModeU = targetAddrMode;
  samplerInfo.addressModeV = targetAddrMode;
  samplerInfo.addressModeW = targetAddrMode;

  auto const &device = VulkanApplicationContext::getInstance()->getDevice();
  vkCreateSampler(device, &samplerInfo, nullptr, &_vkSampler);
}

Sampler::~Sampler() {
  auto const &device = VulkanApplicationContext::getInstance()->getDevice();
  vkDestroySampler(device, _vkSampler, nullptr);
}
