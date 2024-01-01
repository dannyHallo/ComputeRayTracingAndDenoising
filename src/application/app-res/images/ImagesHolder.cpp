#include "ImagesHolder.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "utils/config/RootDir.h"

ImagesHolder::ImagesHolder(VulkanApplicationContext *appContext) : _appContext(appContext) {}

void ImagesHolder::init() {
  _createBlueNoiseImages();
  _createSwapchainRelatedImages();
  _createImageForwardingPairs();
}

void ImagesHolder::onSwapchainResize() {
  _createSwapchainRelatedImages();
  _createImageForwardingPairs();
}

void ImagesHolder::_createSwapchainRelatedImages() {
  _createFrameBufferSizedImages();
  _createStratumResolutionImages();
}

void ImagesHolder::_createBlueNoiseImages() {
  std::vector<std::string> filenames{};

  for (int i = 0; i < 64; i++) {
    filenames.emplace_back(kPathToResourceFolder +
                           "/textures/stbn/vec2_2d_1d/"
                           "stbn_vec2_2Dx1D_128x128x64_" +
                           std::to_string(i) + ".png");
  }
  _vec2BlueNoise = std::make_unique<Image>(filenames, VK_IMAGE_USAGE_STORAGE_BIT |
                                                          VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  for (int i = 0; i < 64; i++) {
    filenames.emplace_back(kPathToResourceFolder +
                           "/textures/stbn/unitvec3_cosine_2d_1d/"
                           "stbn_unitvec3_cosine_2Dx1D_128x128x64_" +
                           std::to_string(i) + ".png");
  }
  _weightedCosineBlueNoise = std::make_unique<Image>(
      filenames, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}

void ImagesHolder::_createFrameBufferSizedImages() {
  auto w = _appContext->getSwapchainExtentWidth();
  auto h = _appContext->getSwapchainExtentHeight();

  _targetImage = std::make_unique<Image>(w, h, VK_FORMAT_R8G8B8A8_UNORM,
                                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  // creating forwarding pairs to copy the image result each frame to a specific swapchain
  _targetForwardingPairs.clear();
  for (int i = 0; i < _appContext->getSwapchainSize(); i++) {
    _targetForwardingPairs.emplace_back(std::make_unique<ImageForwardingPair>(
        _targetImage->getVkImage(), _appContext->getSwapchainImages()[i], VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  }

  _rawImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _seedImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_USAGE_STORAGE_BIT);

  _aTrousInputImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _aTrousOutputImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _positionImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _depthImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _depthImagePrev = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _normalImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _normalImagePrev =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _gradientImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _gradientImagePrev = std::make_unique<Image>(
      w, h, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _varianceHistImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _varianceHistImagePrev =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _meshHashImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _meshHashImagePrev = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _varianceImage = std::make_unique<Image>(w, h, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _lastFrameAccumImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);
}

void ImagesHolder::_createStratumResolutionImages() {
  auto w = _appContext->getSwapchainExtentWidth();
  auto h = _appContext->getSwapchainExtentHeight();

  uint32_t perStratumImageWidth  = ceil(static_cast<float>(w) / 3.0F);
  uint32_t perStratumImageHeight = ceil(static_cast<float>(h) / 3.0F);
  _stratumOffsetImage =
      std::make_unique<Image>(perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _perStratumLockingImage =
      std::make_unique<Image>(perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32_UINT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _visibilityImage = std::make_unique<Image>(
      perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _seedVisibilityImage = std::make_unique<Image>(
      perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_UINT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _temporalGradientNormalizationImagePing = std::make_unique<Image>(
      perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _temporalGradientNormalizationImagePong = std::make_unique<Image>(
      perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}

void ImagesHolder::_createImageForwardingPairs() {
  _aTrousForwardingPair = std::make_unique<ImageForwardingPair>(
      _aTrousOutputImage->getVkImage(), _aTrousInputImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _depthForwardingPair = std::make_unique<ImageForwardingPair>(
      _depthImage->getVkImage(), _depthImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _normalForwardingPair = std::make_unique<ImageForwardingPair>(
      _normalImage->getVkImage(), _normalImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _gradientForwardingPair = std::make_unique<ImageForwardingPair>(
      _gradientImage->getVkImage(), _gradientImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _varianceHistForwardingPair = std::make_unique<ImageForwardingPair>(
      _varianceHistImage->getVkImage(), _varianceHistImagePrev->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  _meshHashForwardingPair = std::make_unique<ImageForwardingPair>(
      _meshHashImage->getVkImage(), _meshHashImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
}
