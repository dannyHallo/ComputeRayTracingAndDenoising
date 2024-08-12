#include "Sampler.hpp"

#include "app-context/VulkanApplicationContext.hpp"

Sampler::Sampler(VulkanApplicationContext *appContext, Sampler::Settings const &settings)
    : _appContext(appContext) {
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

  samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(settings.addressModeU);
  samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(settings.addressModeV);
  samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(settings.addressModeW);

  auto const &device = _appContext->getDevice();
  vkCreateSampler(device, &samplerInfo, nullptr, &_vkSampler);
}

Sampler::~Sampler() {
  auto const &device = _appContext->getDevice();
  vkDestroySampler(device, _vkSampler, nullptr);
}
