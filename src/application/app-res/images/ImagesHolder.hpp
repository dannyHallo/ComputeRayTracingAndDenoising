#pragma once

#include "memory/Image.hpp"

#include <memory>
#include <vector>

class VulkanApplicationContext;
class ImagesHolder {
public:
  static ImagesHolder *getInstance();

  void init(VulkanApplicationContext *appContext);
  void recreateDueToSwapchainResize(VulkanApplicationContext *appContext);

  // getters
  Image *getVec2BlueNoise() const { return _vec2BlueNoise.get(); }
  Image *getWeightedCosineBlueNoise() const { return _weightedCosineBlueNoise.get(); }

  Image *getPositionImage() const { return _positionImage.get(); }
  Image *getRawImage() const { return _rawImage.get(); }

  Image *getTargetImage() const { return _targetImage.get(); }
  ImageForwardingPair *getTargetForwardingPair(uint32_t index) const {
    return _targetForwardingPairs[index].get();
  }

  Image *getLastFrameAccumImage() const { return _lastFrameAccumImage.get(); }

  Image *getVarianceImage() const { return _varianceImage.get(); }

  Image *getStratumOffsetImage() const { return _stratumOffsetImage.get(); }
  Image *getPerStratumLockingImage() const { return _perStratumLockingImage.get(); }

  Image *getVisibilityImage() const { return _visibilityImage.get(); }
  Image *getVisibilitySeedImage() const { return _visibilitySeedImage.get(); }

  Image *getSeedImage() const { return _seedImage.get(); }
  Image *getSeedVisibilityImage() const { return _seedVisibilityImage.get(); }

  Image *getTemporalGradientNormalizationImagePing() const {
    return _temporalGradientNormalizationImagePing.get();
  }
  Image *getTemporalGradientNormalizationImagePong() const {
    return _temporalGradientNormalizationImagePong.get();
  }

  Image *getDepthImage() const { return _depthImage.get(); }
  Image *getDepthImagePrev() const { return _depthImagePrev.get(); }
  ImageForwardingPair *getDepthForwardingPair() const { return _depthForwardingPair.get(); }

  Image *getNormalImage() const { return _normalImage.get(); }
  Image *getNormalImagePrev() const { return _normalImagePrev.get(); }
  ImageForwardingPair *getNormalForwardingPair() const { return _normalForwardingPair.get(); }

  Image *getGradientImage() const { return _gradientImage.get(); }
  Image *getGradientImagePrev() const { return _gradientImagePrev.get(); }
  ImageForwardingPair *getGradientForwardingPair() const { return _gradientForwardingPair.get(); }

  Image *getVarianceHistImage() const { return _varianceHistImage.get(); }
  Image *getVarianceHistImagePrev() const { return _varianceHistImagePrev.get(); }
  ImageForwardingPair *getVarianceHistForwardingPair() const {
    return _varianceHistForwardingPair.get();
  }

  Image *getMeshHashImage() const { return _meshHashImage.get(); }
  Image *getMeshHashImagePrev() const { return _meshHashImagePrev.get(); }
  ImageForwardingPair *getMeshHashForwardingPair() const { return _meshHashForwardingPair.get(); }

  Image *getATrousInputImage() const { return _aTrousInputImage.get(); }
  Image *getATrousOutputImage() const { return _aTrousOutputImage.get(); }
  ImageForwardingPair *getATrousForwardingPair() const { return _aTrousForwardingPair.get(); }

private:
  ImagesHolder();

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

  void _createSwapchainRelatedImages(VulkanApplicationContext *appContext);

  void _createBlueNoiseImages();
  void _createFrameBufferSizedImages(VulkanApplicationContext *appContext);
  void _createStratumResolutionImages(VulkanApplicationContext *appContext);

  void _createImageForwardingPairs();
};