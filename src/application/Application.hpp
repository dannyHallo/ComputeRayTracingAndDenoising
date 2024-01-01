#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "app-context/VulkanApplicationContext.hpp"

#include "ray-tracing/RtScene.hpp"
#include "scene/Mesh.hpp"
#include "utils/camera/Camera.hpp"
#include "utils/logger/Logger.hpp"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

class BuffersHolder;
class ImagesHolder;
class ModelsHolder;
class Application {
  // data alignment in c++ side to meet Vulkan specification:
  // A scalar of size N has a base alignment of N.
  // A three- or four-component vector, with components of size N, has a base
  // alignment of 4 N.
  // https://fvcaputo.github.io/2019/02/06/memory-alignment.html

  // GradientProjectionUniformBufferObject
  bool _useGradientProjection = true;

  bool _movingLightSource = false;
  uint32_t _outputType    = 1; // combined, direct only, indirect only
  float _offsetX          = 0.f;
  float _offsetY          = 0.f;

  // StratumFilterUniformBufferObject
  bool _useStratumFiltering = false;

  // TemporalFilterUniformBufferObject
  bool _useTemporalBlend = true;
  bool _useDepthTest     = false;
  float _depthThreshold  = 0.07;
  bool _useNormalTest    = true;
  float _normalThreshold = 0.99;
  float _blendingAlpha   = 0.15;

  // VarianceUniformBufferObject
  bool _useVarianceEstimation = true;
  bool _skipStoppingFunctions = false;
  bool _useTemporalVariance   = true;
  int _varianceKernelSize     = 4;
  float _variancePhiGaussian  = 1.f;
  float _variancePhiDepth     = 0.2f;

  // BlurFilterUniformBufferObject
  bool _useATrous                       = true;
  int _iCap                             = 5;
  bool _useVarianceGuidedFiltering      = true;
  bool _useGradientInDepth              = true;
  float _phiLuminance                   = 0.3f;
  float _phiDepth                       = 0.2f;
  float _phiNormal                      = 128.f;
  bool _ignoreLuminanceAtFirstIteration = true;
  bool _changingLuminancePhi            = true;
  bool _useJittering                    = true;

  // PostProcessingUniformBufferObject
  uint32_t _displayType = 2;

public:
  Application();
  ~Application();

  // disable move and copy
  Application(const Application &)            = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&)                 = delete;
  Application &operator=(Application &&)      = delete;

  static Camera *getCamera();
  void run();

private:
  float _fps = 0;

  // atrous twicking

  // delta time and last recorded frame time
  float _deltaTime = 0, _frameRecordLastTime = 0;

  Logger _logger;

  static std::unique_ptr<Camera> _camera;
  static std::unique_ptr<Window> _window;

  VulkanApplicationContext *_appContext;

  /// the following resources are NOT swapchain dim related
  // scene for ray tracing
  std::unique_ptr<GpuModel::RtScene> _rtScene;

  // command buffers for rendering and GUI
  std::vector<VkCommandBuffer> _commandBuffers;
  std::vector<VkCommandBuffer> _guiCommandBuffers;

  // render pass for GUI
  VkRenderPass _guiPass = VK_NULL_HANDLE;

  // descriptor pool for GUI
  VkDescriptorPool _guiDescriptorPool = VK_NULL_HANDLE;

  // semaphores and fences for synchronization
  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _framesInFlightFences;

  // framebuffers for GUI
  std::vector<VkFramebuffer> _guiFrameBuffers;

  std::unique_ptr<BuffersHolder> _buffersHolder;
  std::unique_ptr<ImagesHolder> _imagesHolder;
  std::unique_ptr<ModelsHolder> _modelsHolder;

  void _createScene();

  // update uniform buffer object for each frame and save last mvpe matrix
  void _updateScene(uint32_t currentImage);

  void _createRenderCommandBuffers();

  void _createSemaphoresAndFences();

  void _createGuiCommandBuffers();
  void _createGuiRenderPass();
  void _createFramebuffers();
  void _createGuiDescripterPool();

  void _cleanupSwapchainDimensionRelatedResources();
  void _cleanupBufferBundles();
  void _cleanupFrameBuffers();
  void _cleanupRenderCommandBuffers();

  void _createSwapchainDimensionRelatedResources();

  // initialize GUI
  void _initGui();

  // record command buffer for GUI
  void _recordGuiCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex);

  void _waitForTheWindowToBeResumed();

  // draw a frame
  void _drawFrame();

  // prepare GUI
  void _prepareGui();

  // main loop
  void _mainLoop();

  // initialize Vulkan
  void _init();

  // cleanup resources
  void _cleanup();

  bool _needToToggleWindowStyle();
};