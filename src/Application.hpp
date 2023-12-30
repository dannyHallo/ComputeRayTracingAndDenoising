#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "app-context/VulkanApplicationContext.h"

#include "ray-tracing/RtScene.hpp"
#include "scene/ComputeModel.hpp"
#include "scene/Mesh.hpp"
#include "utils/Camera.hpp"
#include "utils/Glm.hpp"
#include "utils/Logger.hpp"
#include "utils/RootDir.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

class Application {
  // data alignment in c++ side to meet Vulkan specification:
  // A scalar of size N has a base alignment of N.
  // A three- or four-component vector, with components of size N, has a base
  // alignment of 4 N.
  // https://fvcaputo.github.io/2019/02/06/memory-alignment.html

  struct GlobalUniformBufferObject {
    alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camPosition;
    alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camFront;
    alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camUp;
    alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camRight;
    uint32_t swapchainWidth;
    uint32_t swapchainHeight;
    float vfov;
    uint32_t currentSample;
    float time;
  };

  struct GradientProjectionUniformBufferObject {
    int bypassGradientProjection;
    alignas(sizeof(glm::vec3::x) * 4) glm::mat4 thisMvpe;
    // alignas(sizeof(glm::dvec3::x) * 4) glm::dmat4 thisMvpe; // use glm::dmat4 for double
  };
  bool mUseGradientProjection = true;

  struct RtxUniformBufferObject {
    uint32_t numTriangles;
    uint32_t numLights;
    int movingLightSource;
    uint32_t outputType;
    float offsetX;
    float offsetY;
  };
  bool mMovingLightSource = false;
  uint32_t mOutputType    = 1; // combined, direct only, indirect only
  float mOffsetX          = 0.f;
  float mOffsetY          = 0.f;

  struct StratumFilterUniformBufferObject {
    int i;
    int bypassStratumFiltering;
  };
  bool mUseStratumFiltering = false;

  struct TemporalFilterUniformBufferObject {
    int bypassTemporalFiltering;
    int useNormalTest;
    float normalThrehold;
    float blendingAlpha;
    alignas(sizeof(glm::vec3::x) * 4) glm::mat4 lastMvpe;
  };
  bool mUseTemporalBlend = true;
  bool mUseDepthTest     = false;
  float mDepthThreshold  = 0.07;
  bool mUseNormalTest    = true;
  float mNormalThreshold = 0.99;
  float mBlendingAlpha   = 0.15;

  struct VarianceUniformBufferObject {
    int bypassVarianceEstimation;
    int skipStoppingFunctions;
    int useTemporalVariance;
    int kernelSize;
    float phiGaussian;
    float phiDepth;
  };
  bool mUseVarianceEstimation = true;
  bool mSkipStoppingFunctions = false;
  bool mUseTemporalVariance   = true;
  int mVarianceKernelSize     = 4;
  float mVariancePhiGaussian  = 1.f;
  float mVariancePhiDepth     = 0.2f;

  struct BlurFilterUniformBufferObject {
    int bypassBluring;
    int i;
    int iCap;
    int useVarianceGuidedFiltering;
    int useGradientInDepth;
    float phiLuminance;
    float phiDepth;
    float phiNormal;
    int ignoreLuminanceAtFirstIteration;
    int changingLuminancePhi;
    int useJittering;
  };
  bool mUseATrous                       = true;
  int mICap                             = 5;
  bool mUseVarianceGuidedFiltering      = true;
  bool mUseGradientInDepth              = true;
  float mPhiLuminance                   = 0.3f;
  float mPhiDepth                       = 0.2f;
  float mPhiNormal                      = 128.f;
  bool mIgnoreLuminanceAtFirstIteration = true;
  bool mChangingLuminancePhi            = true;
  bool mUseJittering                    = true;

  struct PostProcessingUniformBufferObject {
    uint32_t displayType;
  };
  uint32_t mDisplayType = 2;

public:
  Application();
  ~Application() = default;

  // disable move and copy
  Application(const Application &)            = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&)                 = delete;
  Application &operator=(Application &&)      = delete;

  static Camera *getCamera();
  void run();

private:
  float mFps = 0;

  // atrous twicking

  // delta time and last recorded frame time
  float mDeltaTime = 0, mFrameRecordLastTime = 0;

  Logger mLogger;

  static std::unique_ptr<Camera> mCamera;
  static std::unique_ptr<Window> mWindow;

  VulkanApplicationContext *mAppContext;

  /// the following resources are NOT swapchain dim related
  // scene for ray tracing
  std::unique_ptr<GpuModel::Scene> _rtScene;

  std::unique_ptr<ComputeModel> _gradientProjectionModel;
  std::unique_ptr<ComputeModel> _rtxModel;
  std::unique_ptr<ComputeModel> _gradientModel;
  std::vector<std::unique_ptr<ComputeModel>> _stratumFilterModels;
  std::unique_ptr<ComputeModel> _temporalFilterModel;
  std::unique_ptr<ComputeModel> _varianceModel;
  std::vector<std::unique_ptr<ComputeModel>> _aTrousModels;
  std::unique_ptr<ComputeModel> _postProcessingModel;

  // buffer bundles for ray tracing, temporal filtering, and blur filtering
  std::unique_ptr<BufferBundle> _globalBufferBundle;
  std::unique_ptr<BufferBundle> _gradientProjectionBufferBundle;
  std::unique_ptr<BufferBundle> _rtxBufferBundle;
  std::unique_ptr<BufferBundle> _temperalFilterBufferBundle;
  std::vector<std::unique_ptr<BufferBundle>> _stratumFilterBufferBundle;
  std::unique_ptr<BufferBundle> _varianceBufferBundle;
  std::vector<std::unique_ptr<BufferBundle>> _blurFilterBufferBundles;
  std::unique_ptr<BufferBundle> _postProcessingBufferBundle;

  std::unique_ptr<BufferBundle> _triangleBufferBundle;
  std::unique_ptr<BufferBundle> _materialBufferBundle;
  std::unique_ptr<BufferBundle> _bvhBufferBundle;
  std::unique_ptr<BufferBundle> _lightsBufferBundle;

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

  // framebuffers for GUI
  std::vector<VkFramebuffer> _guiFrameBuffers;

  void _createScene();
  void _createBufferBundles();
  void _createImagesAndForwardingPairs();
  void _createComputeModels();

  // update uniform buffer object for each frame and save last mvpe matrix
  void _updateScene(uint32_t currentImage);

  void _createRenderCommandBuffers();

  void _vkCreateSemaphoresAndFences();

  void _createGuiCommandBuffers();

  void _createGuiRenderPass();

  void _createGuiFramebuffers();

  void _createGuiDescripterPool();

  void _cleanupSwapchainDimensionRelatedResources();
  void _cleanupImagesAndForwardingPairs();
  void _cleanupBufferBundles();
  void _cleanupGuiFrameBuffers();
  void _cleanupComputeModels();
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