#pragma once

#include "SvoTracerDataGpu.hpp"
#include "SvoTracerTweakingData.hpp"
#include "glm/glm.hpp" // IWYU pragma: export
#include "scheduler/Scheduler.hpp"
#include "volk.h"

#include <memory>
#include <vector>

class Logger;
class VulkanApplicationContext;

class Image;
class ImageForwardingPair;
class Buffer;
class BufferBundle;
class Sampler;
class SvoBuilder;
class DescriptorSetBundle;
class ComputePipeline;
class Camera;
class ShaderCompiler;
class ShaderChangeListener;
class TomlConfigReader;

class SvoTracer : public Scheduler {
public:
  SvoTracer(VulkanApplicationContext *appContext, Logger *logger, size_t framesInFlight,
            Camera *camera, ShaderCompiler *shaderCompiler,
            ShaderChangeListener *shaderChangeListener, TomlConfigReader *tomlConfigReader);
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
  SvoTracerTweakingData &getTweakingData() { return _tweakingData; }

  G_OutputInfo getOutputInfo();

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  Camera *_camera;
  ShaderCompiler *_shaderCompiler;
  ShaderChangeListener *_shaderChangeListener;
  TomlConfigReader *_tomlConfigReader;

  SvoTracerTweakingData _tweakingData;

  SvoBuilder *_svoBuilder = nullptr;

  // config
  uint32_t _aTrousSizeMax{};
  uint32_t _beamResolution{};
  uint32_t _taaSamplingOffsetSize{};
  float _upscaleRatio{};
  void _loadConfig();

  size_t _framesInFlight;
  std::vector<VkCommandBuffer> _tracingCommandBuffers{};
  std::vector<VkCommandBuffer> _deliveryCommandBuffers{};

  uint32_t _lowResWidth   = 0;
  uint32_t _lowResHeight  = 0;
  uint32_t _highResWidth  = 0;
  uint32_t _highResHeight = 0;

  std::vector<glm::vec2> _subpixOffsets{};

  void _updateImageResolutions();

  void _recordRenderingCommandBuffers();
  void _recordDeliveryCommandBuffers();

  void _createTaaSamplingOffsets();

  /// IMAGES

  std::unique_ptr<Sampler> _defaultSampler;
  std::unique_ptr<Sampler> _skyLutSampler;

  // spatial-temporal blue noise (arrays of images)
  std::unique_ptr<Image> _vec2BlueNoise;
  std::unique_ptr<Image> _vec3BlueNoise;
  std::unique_ptr<Image> _weightedCosineBlueNoise;

  std::unique_ptr<Image> _transmittanceLutImage;
  std::unique_ptr<Image> _multiScatteringLutImage;
  std::unique_ptr<Image> _skyViewLutImage;

  // the followed up resources are swapchain dimension related
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

  std::unique_ptr<Image> _taaImage;
  std::unique_ptr<Image> _lastTaaImage;
  std::unique_ptr<ImageForwardingPair> _taaForwardingPair;

  std::unique_ptr<Image> _blittedImage;

  std::unique_ptr<Image> _aTrousPingImage;
  std::unique_ptr<Image> _aTrousPongImage;

  std::unique_ptr<Image> _renderTargetImage;
  std::vector<std::unique_ptr<ImageForwardingPair>> _targetForwardingPairs;

  void _createSamplers();

  void _createImages();
  void _createSkyLutImages();
  void _createSwapchainRelatedImages(); // auto release

  void _createBlueNoiseImages();
  void _createFullSizedImages();
  void _createImageForwardingPairs();

  /// BUFFERS

  // You seem to be saying you keep #FIF copies of all resources. Only resources that are actively
  // accessed by both the CPU and GPU need to because the GPU will not work on multiple frames at
  // the same time. You don't need to multiply textures or buffers in device memory. This means that
  // the memory increase should be small and solves your problem with uploading data:
  // https://www.reddit.com/r/vulkan/comments/10io2l8/is_framesinflight_fif_method_really_worth_it/

  std::unique_ptr<BufferBundle> _renderInfoBufferBundle;
  std::unique_ptr<BufferBundle> _environmentInfoBufferBundle;
  std::unique_ptr<BufferBundle> _tweakableParametersBufferBundle;
  std::unique_ptr<BufferBundle> _temporalFilterInfoBufferBundle;
  std::unique_ptr<BufferBundle> _spatialFilterInfoBufferBundle;

  std::unique_ptr<Buffer> _sceneInfoBuffer;
  std::unique_ptr<Buffer> _aTrousIterationBuffer;
  std::vector<std::unique_ptr<Buffer>> _aTrousIterationStagingBuffers;
  std::unique_ptr<Buffer> _outputInfoBuffer;

  void _createBuffersAndBufferBundles();
  void _initBufferData();

  /// PIPELINES

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

  std::unique_ptr<ComputePipeline> _transmittanceLutPipeline;
  std::unique_ptr<ComputePipeline> _multiScatteringLutPipeline;
  std::unique_ptr<ComputePipeline> _skyViewLutPipeline;

  std::unique_ptr<ComputePipeline> _svoCourseBeamPipeline;
  std::unique_ptr<ComputePipeline> _svoTracingPipeline;
  std::unique_ptr<ComputePipeline> _temporalFilterPipeline;
  std::unique_ptr<ComputePipeline> _aTrousPipeline;
  std::unique_ptr<ComputePipeline> _backgroundBlitPipeline;
  std::unique_ptr<ComputePipeline> _taaUpscalingPipeline;
  std::unique_ptr<ComputePipeline> _postProcessingPipeline;

  void _createDescriptorSetBundle();
  void _createPipelines();
  void _updatePipelinesDescriptorBundles();
};