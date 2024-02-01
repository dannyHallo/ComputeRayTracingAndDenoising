#pragma once

#include "SvoTracerDataGpu.hpp"
#include "utils/incl/Glm.hpp"
#include "utils/incl/Vulkan.hpp"

#include <memory>

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

class SvoTracer {
public:
  SvoTracer(VulkanApplicationContext *appContext, Logger *logger, size_t framesInFlight,
            Camera *camera);
  ~SvoTracer();

  // disable copy and move
  SvoTracer(SvoTracer const &)            = delete;
  SvoTracer(SvoTracer &&)                 = delete;
  SvoTracer &operator=(SvoTracer const &) = delete;
  SvoTracer &operator=(SvoTracer &&)      = delete;

  void init(SvoBuilder *svoBuilder);

  void onSwapchainResize();
  VkCommandBuffer getCommandBuffer(size_t currentFrame) { return _commandBuffers[currentFrame]; }

  void updateUboData(size_t currentFrame);

  // this getter will be used for imguiManager
  SvoTracerDataGpu &getUboData() { return _uboData; }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  Camera *_camera;
  SvoBuilder*_svoBuilder = nullptr;

  size_t _framesInFlight;
  std::vector<VkCommandBuffer> _commandBuffers{};

  SvoTracerDataGpu _uboData;

  void _recordCommandBuffers();

  /// IMAGES

  // spatial-temporal blue noise
  std::unique_ptr<Image> _vec2BlueNoise;
  std::unique_ptr<Image> _weightedCosineBlueNoise;

  // following resources are swapchain dim related
  std::unique_ptr<Image> _beamDepthImage;

  std::unique_ptr<Image> _positionImage;
  std::unique_ptr<Image> _rawImage;

  std::unique_ptr<Image> _renderTargetImage;
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

  void _createImages();
  void _createOneTimeImages();
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

  std::unique_ptr<BufferBundle> _renderInfoUniformBuffers;
  std::unique_ptr<BufferBundle> _twickableParametersUniformBuffers;

  std::unique_ptr<Buffer> _sceneDataBuffer;

  void _createBuffersAndBufferBundles();
  void _initBufferData();

  /// PIPELINES

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

  // std::unique_ptr<ComputePipeline> _gradientProjectionPipeline;
  std::unique_ptr<ComputePipeline> _svoCourseBeamPipeline;
  std::unique_ptr<ComputePipeline> _svoTracingPipeline;
  // std::unique_ptr<ComputePipeline> _gradientPipeline;
  // std::vector<std::unique_ptr<ComputePipeline>> _stratumFilterPipelines;
  // std::unique_ptr<ComputePipeline> _temporalFilterPipeline;
  // std::unique_ptr<ComputePipeline> _variancePipeline;
  // std::vector<std::unique_ptr<ComputePipeline>> _aTrousPipelines;
  std::unique_ptr<ComputePipeline> _postProcessingPipeline;

  void _createDescriptorSetBundle();
  void _createPipelines();
};