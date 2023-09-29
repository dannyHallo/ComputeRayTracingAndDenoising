#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "app-context/VulkanApplicationContext.h"

#include "ray-tracing/RtScene.h"
#include "scene/ComputeModel.h"
#include "scene/Mesh.h"
#include "utils/Camera.h"
#include "utils/Logger.h"
#include "utils/RootDir.h"
#include "utils/glm.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

class Application {
  // data alignment in c++ side to meet Vulkan specification:
  // A scalar of size N has a base alignment of N.
  // A three- or four-component vector, with components of size N, has a base
  // alignment of 4 N.
  // https://fvcaputo.github.io/2019/02/06/memory-alignment.html
  struct RtxUniformBufferObject {
    alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camPosition;
    alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camFront;
    alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camUp;
    alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camRight;
    float vfov;
    float time;
    uint32_t currentSample;
    uint32_t numTriangles;
    uint32_t numLights;
  };

  struct TemporalFilterUniformBufferObject {
    int bypassTemporalFiltering;
    int useDepthTest;
    float depthThrehold;
    int useNormalTest;
    float normalThrehold;
    float blendingAlpha;
    alignas(sizeof(glm::vec3::x) * 4) glm::mat4 lastMvpe;
    uint swapchainWidth; // TODO: remove this
    uint swapchainHeight;
  };

  struct VarianceUniformBufferObject {
    int bypassVarianceEstimation;
    int skipStoppingFunctions;
    int useTemporalVariance;
    int kernelSize;
    float phiGaussian;
    float phiDepth;
  };

  struct BlurFilterUniformBufferObject {
    int bypassBluring;
    int i;
    int iCap;
    int showVariance;
    int useVarianceGuidedFiltering;
    float phiLuminance;
    float phiDepth;
    float phiNormal;
    float centralKernelWeight;
    int useThreeByThreeKernel;
    int ignoreLuminanceAtFirstIteration;
    int changingLuminancePhi;
  };

  float mFps = 0;

  // whether to use temporal and blur filtering
  bool mUseATrous = true;

  // temporal filter
  bool mUseTemporalBlend = true;
  bool mUseDepthTest     = false;
  float mDepthThreshold  = 0.07;
  bool mUseNormalTest    = true;
  float mNormalThreshold = 0.99;
  float mBlendingAlpha   = 0.15;

  // variance estimation
  bool mUseVarianceEstimation = true;
  bool mSkipStoppingFunctions = false;
  bool mUseTemporalVariance   = true;
  int mVarianceKernelSize     = 4;
  float mVariancePhiGaussian  = 1.f;
  float mVariancePhiDepth     = 0.2f;

  // atrous twicking
  int mICap                             = 5;
  bool mShowVariance                    = false;
  bool mUseVarianceGuidedFiltering      = true;
  float mPhiLuminance                   = 0.5f;
  float mPhiDepth                       = 0.2f;
  float mPhiNormal                      = 128.f;
  float mCentralKernelWeight            = 0.5f;
  bool mUseThreeByThreeKernel           = true;
  bool mIgnoreLuminanceAtFirstIteration = true;
  bool mChangingLuminancePhi            = true;

  // delta time and last recorded frame time
  float mDeltaTime = 0, mFrameRecordLastTime = 0;

  static std::unique_ptr<Camera> mCamera;
  static std::unique_ptr<Window> mWindow;

  VulkanApplicationContext *mAppContext;

  /// the following resources are NOT swapchain dim related
  // scene for ray tracing
  std::unique_ptr<GpuModel::Scene> mRtScene;

  // compute models for ray tracing, temporal filtering, and blur filtering
  std::unique_ptr<ComputeModel> mRtxModel;
  std::unique_ptr<ComputeModel> mGradientModel;
  std::unique_ptr<ComputeModel> mTemporalFilterModel;
  std::unique_ptr<ComputeModel> mVarianceModel;
  std::vector<std::unique_ptr<ComputeModel>> mATrousModels;
  std::unique_ptr<ComputeModel> mPostProcessingModel;

  // buffer bundles for ray tracing, temporal filtering, and blur filtering
  std::unique_ptr<BufferBundle> mRtxBufferBundle;
  std::unique_ptr<BufferBundle> mTemperalFilterBufferBundle;
  std::unique_ptr<BufferBundle> mVarianceBufferBundle;
  std::vector<std::unique_ptr<BufferBundle>> mBlurFilterBufferBundles;

  std::unique_ptr<BufferBundle> mTriangleBufferBundle;
  std::unique_ptr<BufferBundle> mMaterialBufferBundle;
  std::unique_ptr<BufferBundle> mBvhBufferBundle;
  std::unique_ptr<BufferBundle> mLightsBufferBundle;

  // command buffers for rendering and GUI
  std::vector<VkCommandBuffer> mCommandBuffers;
  std::vector<VkCommandBuffer> mGuiCommandBuffers;

  // render pass for GUI
  VkRenderPass mGuiPass = VK_NULL_HANDLE;

  // descriptor pool for GUI
  VkDescriptorPool mGuiDescriptorPool = VK_NULL_HANDLE;

  // semaphores and fences for synchronization
  std::vector<VkSemaphore> mImageAvailableSemaphores;
  std::vector<VkSemaphore> mRenderFinishedSemaphores;
  std::vector<VkFence> mFramesInFlightFences;

  /// the following resources ARE swapchain dim related
  // images for ray tracing and post-processing
  std::unique_ptr<Image> mPositionImage;
  std::unique_ptr<Image> mRawImage;

  std::unique_ptr<Image> mTargetImage;
  std::vector<std::unique_ptr<ImageForwardingPair>> mTargetForwardingPairs;

  std::unique_ptr<Image> mLastFrameAccumImage;

  std::unique_ptr<Image> mVarianceImage;

  std::unique_ptr<Image> mDepthImage;
  std::unique_ptr<Image> mDepthImagePrev;
  std::unique_ptr<ImageForwardingPair> mDepthForwardingPair;

  std::unique_ptr<Image> mNormalImage;
  std::unique_ptr<Image> mNormalImagePrev;
  std::unique_ptr<ImageForwardingPair> mNormalForwardingPair;

  std::unique_ptr<Image> mGradientImage;
  std::unique_ptr<Image> mGradientImagePrev;
  std::unique_ptr<ImageForwardingPair> mGradientForwardingPair;

  std::unique_ptr<Image> mVarianceHistImage;
  std::unique_ptr<Image> mVarianceHistImagePrev;
  std::unique_ptr<ImageForwardingPair> mVarianceHistForwardingPair;

  std::unique_ptr<Image> mMeshHashImage;
  std::unique_ptr<Image> mMeshHashImagePrev;
  std::unique_ptr<ImageForwardingPair> mMeshHashForwardingPair;

  std::unique_ptr<Image> mATrousInputImage;
  std::unique_ptr<Image> mATrousOutputImage;
  std::unique_ptr<ImageForwardingPair> mATrousForwardingPair;

  // framebuffers for GUI
  std::vector<VkFramebuffer> mGuiFrameBuffers;

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
  void createScene();
  void createBufferBundles();
  void createImagesAndForwardingPairs();
  void createComputeModels();

  // update uniform buffer object for each frame and save last mvpe matrix
  void updateScene(uint32_t currentImage);

  void createRenderCommandBuffers();

  void vkCreateSemaphoresAndFences();

  void createGuiCommandBuffers();

  void createGuiRenderPass();

  void createGuiFramebuffers();

  void createGuiDescripterPool();

  void cleanupSwapchainDimensionRelatedResources();
  void cleanupImagesAndForwardingPairs();
  void cleanupBufferBundles();
  void cleanupGuiFrameBuffers();
  void cleanupComputeModels();
  void cleanupRenderCommandBuffers();

  void createSwapchainDimensionRelatedResources();

  // initialize GUI
  void initGui();

  // record command buffer for GUI
  void recordGuiCommandBuffer(VkCommandBuffer &commandBuffer,
                              uint32_t imageIndex);

  void waitForTheWindowToBeResumed();

  // draw a frame
  void drawFrame();

  // prepare GUI
  void prepareGui();

  // main loop
  void mainLoop();

  // initialize Vulkan
  void init();

  // cleanup resources
  void cleanup();

  bool needToToggleWindowStyle();
};