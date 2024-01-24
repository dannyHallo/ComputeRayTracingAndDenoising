#pragma once

#include "app-context/VulkanApplicationContext.hpp"

#include "utils/logger/Logger.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class Window;
class SvoBuilder;
class BuffersHolder;
class ImagesHolder;
class PipelinesHolder;
class ImguiManager;
class Camera;
class FpsSink;
class Application {
public:
  Application();
  ~Application();

  // disable move and copy
  Application(const Application &)            = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&)                 = delete;
  Application &operator=(Application &&)      = delete;

  Camera *getCamera();
  void run();

private:
  bool _useGradientProjection = true;

  bool _movingLightSource = false;
  uint32_t _outputType    = 1; // combined, direct only, indirect only
  float _offsetX          = 0.F;
  float _offsetY          = 0.F;

  // StratumFilterUniformBufferObject
  bool _useStratumFiltering = false;

  // TemporalFilterUniformBufferObject
  bool _useTemporalBlend = true;
  bool _useDepthTest     = false;
  float _depthThreshold  = 0.07F;
  bool _useNormalTest    = true;
  float _normalThreshold = 0.99F;
  float _blendingAlpha   = 0.15F;

  // VarianceUniformBufferObject
  bool _useVarianceEstimation = true;
  bool _skipStoppingFunctions = false;
  bool _useTemporalVariance   = true;
  int _varianceKernelSize     = 4;
  float _variancePhiGaussian  = 1.F;
  float _variancePhiDepth     = 0.2F;

  // BlurFilterUniformBufferObject
  bool _useATrous                       = true;
  int _iCap                             = 0;
  bool _useVarianceGuidedFiltering      = true;
  bool _useGradientInDepth              = true;
  float _phiLuminance                   = 0.3F;
  float _phiDepth                       = 0.2F;
  float _phiNormal                      = 128.F;
  bool _ignoreLuminanceAtFirstIteration = true;
  bool _changingLuminancePhi            = true;
  bool _useJittering                    = true;

  // PostProcessingUniformBufferObject
  uint32_t _displayType = 0;

  // atrous twicking

  Logger _logger;

  std::unique_ptr<FpsSink> _fpsSink;
  std::unique_ptr<ImguiManager> _imguiManager;
  std::unique_ptr<Camera> _camera;
  std::unique_ptr<Window> _window;

  VulkanApplicationContext *_appContext;

  std::unique_ptr<SvoBuilder> _svoScene;

  // command buffers for rendering and GUI
  std::vector<VkCommandBuffer> _commandBuffers;

  // semaphores and fences for synchronization
  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _framesInFlightFences;

  std::unique_ptr<BuffersHolder> _buffersHolder;
  std::unique_ptr<ImagesHolder> _imagesHolder;
  std::unique_ptr<PipelinesHolder> _pipelinesHolder;

  void _createSvoScene();

  // update uniform buffer object for each frame and save last mvpe matrix
  void _updateUbos(size_t currentFrame);

  void _createRenderCommandBuffers();

  void _createSemaphoresAndFences();

  void _cleanupSwapchainDimensionRelatedResources();
  void _cleanupBufferBundles();
  void _cleanupRenderCommandBuffers();

  void _createSwapchainDimensionRelatedResources();

  void _initGui();

  void _waitForTheWindowToBeResumed();

  void _drawFrame();

  void _mainLoop();

  void _init();

  void _cleanup();

  bool _needToToggleWindowStyle();
};