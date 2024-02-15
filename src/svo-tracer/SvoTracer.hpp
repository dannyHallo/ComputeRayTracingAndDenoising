#pragma once

#include "SvoTracerDataGpu.hpp"
#include "scheduler/Scheduler.hpp"
#include "utils/incl/Glm.hpp"
#include "utils/incl/Vulkan.hpp"

#include <memory>
#include <utility>

class Logger;
class VulkanApplicationContext;

class Image;
class ImageForwardingPair;
class Buffer;
class BufferBundle;
class SvoBuilder;
class DescriptorSetBundle;
class ComputePipeline;
class Camera;
class ShaderChangeListener;

class SvoTracer : public Scheduler {
public:
  SvoTracer(VulkanApplicationContext *appContext, Logger *logger, size_t framesInFlight,
            Camera *camera, ShaderChangeListener *shaderChangeListener);
  ~SvoTracer() override;

  // disable copy and move
  SvoTracer(SvoTracer const &)            = delete;
  SvoTracer(SvoTracer &&)                 = delete;
  SvoTracer &operator=(SvoTracer const &) = delete;
  SvoTracer &operator=(SvoTracer &&)      = delete;

  void init(SvoBuilder *svoBuilder);
  void update() override;

  void onSwapchainResize();
  VkCommandBuffer getTracingCommandBuffer(size_t currentFrame) {
    return _tracingCommandBuffers[currentFrame];
  }
  VkCommandBuffer getDeliveryCommandBuffer(size_t imageIndex) {
    return _deliveryCommandBuffers[imageIndex];
  }

  void updateUboData(size_t currentFrame);

  // this getter will be used for imguiManager
  SvoTracerTweakingData &getUboData() { return _uboData; }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  Camera *_camera;
  ShaderChangeListener *_shaderChangeListener;
  SvoBuilder *_svoBuilder = nullptr;

  size_t _framesInFlight;
  std::vector<VkCommandBuffer> _tracingCommandBuffers{};
  std::vector<VkCommandBuffer> _deliveryCommandBuffers{};

  SvoTracerTweakingData _uboData;

  std::vector<glm::vec2> _taaSamplingOffsets{};

  void _recordRenderingCommandBuffers();
  void _recordDeliveryCommandBuffers();

  void _createTaaSamplingOffsets();

  /// IMAGES

  // spatial-temporal blue noise (arrays of images)
  std::unique_ptr<Image> _vec2BlueNoise;
  std::unique_ptr<Image> _weightedCosineBlueNoise;

  // following resources are swapchain dim related
  std::unique_ptr<Image> _backgroundImage;
  std::unique_ptr<Image> _beamDepthImage;
  std::unique_ptr<Image> _rawImage;
  std::unique_ptr<Image> _depthImage;
  std::unique_ptr<Image> _octreeVisualizationImage;
  std::unique_ptr<Image> _hitImage;
  std::unique_ptr<Image> _temporalHistLengthImage;
  std::unique_ptr<Image> _motionImage;

  std::unique_ptr<Image> _normalImage;
  std::unique_ptr<Image> _lastNormalImage;
  std::unique_ptr<ImageForwardingPair> _normalForwardingPair;

  std::unique_ptr<Image> _positionImage;
  std::unique_ptr<Image> _lastPositionImage;
  std::unique_ptr<ImageForwardingPair> _positionForwardingPair;

  std::unique_ptr<Image> _voxHashImage;
  std::unique_ptr<Image> _lastVoxHashImage;
  std::unique_ptr<ImageForwardingPair> _voxHashForwardingPair;

  std::unique_ptr<Image> _accumedImage;
  std::unique_ptr<Image> _lastAccumedImage;
  std::unique_ptr<ImageForwardingPair> _accumedForwardingPair;

  std::unique_ptr<Image> _varianceHistImage;
  std::unique_ptr<Image> _lastVarianceHistImage;
  std::unique_ptr<ImageForwardingPair> _varianceHistForwardingPair;

  std::unique_ptr<Image> _aTrousPingImage;
  std::unique_ptr<Image> _aTrousPongImage;
  std::unique_ptr<Image> _aTrousFinalResultImage;

  std::unique_ptr<Image> _renderTargetImage;
  std::vector<std::unique_ptr<ImageForwardingPair>> _targetForwardingPairs;

  // std::unique_ptr<Image> _varianceImage;

  // std::unique_ptr<Image> _stratumOffsetImage;
  // std::unique_ptr<Image> _perStratumLockingImage;

  // std::unique_ptr<Image> _visibilityImage;
  // std::unique_ptr<Image> _visibilitySeedImage;

  // std::unique_ptr<Image> _seedImage;
  // std::unique_ptr<Image> _seedVisibilityImage;

  // std::unique_ptr<Image> _temporalGradientNormalizationImagePing;
  // std::unique_ptr<Image> _temporalGradientNormalizationImagePong;

  // std::unique_ptr<Image> _gradientImage;
  // std::unique_ptr<Image> _gradientImagePrev;
  // std::unique_ptr<ImageForwardingPair> _gradientForwardingPair;

  // std::unique_ptr<Image> _aTrousInputImage;
  // std::unique_ptr<Image> _aTrousOutputImage;
  // std::unique_ptr<ImageForwardingPair> _aTrousForwardingPair;

  void _createImages();
  void _createResourseImages();
  void _createSwapchainRelatedImages(); // auto release

  void _createBlueNoiseImages();
  void _createFullSizedImages();
  void _createStratumSizedImages();
  void _createImageForwardingPairs();

  /// BUFFERS

  // You seem to be saying you keep #FIF copies of all resources. Only resources that are actively
  // accessed by both the CPU and GPU need to because the GPU will not work on multiple frames at
  // the same time. You don't need to multiply textures or buffers in device memory. This means that
  // the memory increase should be small and solves your problem with uploading data:
  // https://www.reddit.com/r/vulkan/comments/10io2l8/is_framesinflight_fif_method_really_worth_it/

  std::unique_ptr<BufferBundle> _renderInfoBufferBundle;
  std::unique_ptr<BufferBundle> _environmentInfoBufferBundle;
  std::unique_ptr<BufferBundle> _twickableParametersBufferBundle;
  std::unique_ptr<BufferBundle> _temporalFilterInfoBufferBundle;
  std::unique_ptr<BufferBundle> _spatialFilterInfoBufferBundle;

  std::unique_ptr<Buffer> _sceneInfoBuffer;
  std::unique_ptr<Buffer> _aTrousIterationBuffer;
  std::vector<std::unique_ptr<Buffer>> _aTrousIterationStagingBuffers;

  void _createBuffersAndBufferBundles();
  void _initBufferData();

  /// PIPELINES

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

  // std::unique_ptr<ComputePipeline> _gradientProjectionPipeline;
  std::unique_ptr<ComputePipeline> _svoCourseBeamPipeline;
  std::unique_ptr<ComputePipeline> _svoTracingPipeline;
  // std::unique_ptr<ComputePipeline> _gradientPipeline;
  // std::vector<std::unique_ptr<ComputePipeline>> _stratumFilterPipelines;
  std::unique_ptr<ComputePipeline> _temporalFilterPipeline;
  // std::unique_ptr<ComputePipeline> _variancePipeline;
  std::unique_ptr<ComputePipeline> _aTrousPipeline;
  std::unique_ptr<ComputePipeline> _postProcessingPipeline;

  void _createDescriptorSetBundle();
  void _createPipelines();
  void _updatePipelinesDescriptorBundles();
};