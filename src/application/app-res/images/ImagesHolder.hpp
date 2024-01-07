#pragma once

#include "memory/Image.hpp"

#include <memory>
#include <vector>

class VulkanApplicationContext;
class ImagesHolder {
public:
  ImagesHolder(VulkanApplicationContext *appContext) : _appContext(appContext) {}

  void init();
  void onSwapchainResize();

  [[nodiscard]] Image *getVec2BlueNoise() const { return _vec2BlueNoise.get(); }
  [[nodiscard]] Image *getWeightedCosineBlueNoise() const { return _weightedCosineBlueNoise.get(); }

  [[nodiscard]] Image *getPositionImage() const { return _positionImage.get(); }
  [[nodiscard]] Image *getRawImage() const { return _rawImage.get(); }

  [[nodiscard]] Image *getTargetImage() const { return _targetImage.get(); }
  [[nodiscard]] ImageForwardingPair *getTargetForwardingPair(uint32_t index) const {
    return _targetForwardingPairs[index].get();
  }

  [[nodiscard]] Image *getLastFrameAccumImage() const { return _lastFrameAccumImage.get(); }

  [[nodiscard]] Image *getVarianceImage() const { return _varianceImage.get(); }

  [[nodiscard]] Image *getStratumOffsetImage() const { return _stratumOffsetImage.get(); }
  [[nodiscard]] Image *getPerStratumLockingImage() const { return _perStratumLockingImage.get(); }

  [[nodiscard]] Image *getVisibilityImage() const { return _visibilityImage.get(); }
  [[nodiscard]] Image *getVisibilitySeedImage() const { return _visibilitySeedImage.get(); }

  [[nodiscard]] Image *getSeedImage() const { return _seedImage.get(); }
  [[nodiscard]] Image *getSeedVisibilityImage() const { return _seedVisibilityImage.get(); }

  [[nodiscard]] Image *getTemporalGradientNormalizationImagePing() const {
    return _temporalGradientNormalizationImagePing.get();
  }
  [[nodiscard]] Image *getTemporalGradientNormalizationImagePong() const {
    return _temporalGradientNormalizationImagePong.get();
  }

  [[nodiscard]] Image *getDepthImage() const { return _depthImage.get(); }
  [[nodiscard]] Image *getDepthImagePrev() const { return _depthImagePrev.get(); }
  [[nodiscard]] ImageForwardingPair *getDepthForwardingPair() const {
    return _depthForwardingPair.get();
  }

  [[nodiscard]] Image *getNormalImage() const { return _normalImage.get(); }
  [[nodiscard]] Image *getNormalImagePrev() const { return _normalImagePrev.get(); }
  [[nodiscard]] ImageForwardingPair *getNormalForwardingPair() const {
    return _normalForwardingPair.get();
  }

  [[nodiscard]] Image *getGradientImage() const { return _gradientImage.get(); }
  [[nodiscard]] Image *getGradientImagePrev() const { return _gradientImagePrev.get(); }
  [[nodiscard]] ImageForwardingPair *getGradientForwardingPair() const {
    return _gradientForwardingPair.get();
  }

  [[nodiscard]] Image *getVarianceHistImage() const { return _varianceHistImage.get(); }
  [[nodiscard]] Image *getVarianceHistImagePrev() const { return _varianceHistImagePrev.get(); }
  [[nodiscard]] ImageForwardingPair *getVarianceHistForwardingPair() const {
    return _varianceHistForwardingPair.get();
  }

  [[nodiscard]] Image *getMeshHashImage() const { return _meshHashImage.get(); }
  [[nodiscard]] Image *getMeshHashImagePrev() const { return _meshHashImagePrev.get(); }
  [[nodiscard]] ImageForwardingPair *getMeshHashForwardingPair() const {
    return _meshHashForwardingPair.get();
  }

  [[nodiscard]] Image *getATrousInputImage() const { return _aTrousInputImage.get(); }
  [[nodiscard]] Image *getATrousOutputImage() const { return _aTrousOutputImage.get(); }
  [[nodiscard]] ImageForwardingPair *getATrousForwardingPair() const {
    return _aTrousForwardingPair.get();
  }

private:
  VulkanApplicationContext *_appContext;

  // spatial-temporal blue noise
  std::unique_ptr<Image> _vec2BlueNoise;
  std::unique_ptr<Image> _weightedCosineBlueNoise;

  /// the following resources ARE swapchain dim related
  // images for ray tracing and post-processing
  std::unique_ptr<Image> _positionImage;
  std::unique_ptr<Image> _rawImage;

  std::unique_ptr<Image> _targetImage;
  std::vector<std::unique_ptr<ImageForwardingPair>> _targetForwardingPairs;

  std::unique_ptr<Image> _lastFrameAccumImage;

  std::unique_ptr<Image> _varianceImage;

  std::unique_ptr<Image> _stratumOffsetImage;
  std::unique_ptr<Image> _perStratumLockingImage;

  std::unique_ptr<Image> _visibilityImage;
  std::unique_ptr<Image> _visibilitySeedImage;

  std::unique_ptr<Image> _seedImage;
  std::unique_ptr<Image> _seedVisibilityImage;

  std::unique_ptr<Image> _temporalGradientNormalizationImagePing;
  std::unique_ptr<Image> _temporalGradientNormalizationImagePong;

  std::unique_ptr<Image> _depthImage;
  std::unique_ptr<Image> _depthImagePrev;
  std::unique_ptr<ImageForwardingPair> _depthForwardingPair;

  std::unique_ptr<Image> _normalImage;
  std::unique_ptr<Image> _normalImagePrev;
  std::unique_ptr<ImageForwardingPair> _normalForwardingPair;

  std::unique_ptr<Image> _gradientImage;
  std::unique_ptr<Image> _gradientImagePrev;
  std::unique_ptr<ImageForwardingPair> _gradientForwardingPair;

  std::unique_ptr<Image> _varianceHistImage;
  std::unique_ptr<Image> _varianceHistImagePrev;
  std::unique_ptr<ImageForwardingPair> _varianceHistForwardingPair;

  std::unique_ptr<Image> _meshHashImage;
  std::unique_ptr<Image> _meshHashImagePrev;
  std::unique_ptr<ImageForwardingPair> _meshHashForwardingPair;

  std::unique_ptr<Image> _aTrousInputImage;
  std::unique_ptr<Image> _aTrousOutputImage;
  std::unique_ptr<ImageForwardingPair> _aTrousForwardingPair;

  void _createSwapchainRelatedImages();

  void _createBlueNoiseImages();
  void _createFrameBufferSizedImages();
  void _createStratumResolutionImages();

  void _createImageForwardingPairs();
};