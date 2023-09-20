#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "app-context/VulkanApplicationContext.h"

#include "memory/ImageUtils.h"
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
  // A three- or four-component vector, with components of size N, has a base alignment of 4 N.
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
    alignas(sizeof(glm::vec3::x) * 4) glm::mat4 lastMvpe;
    uint swapchainWidth;
    uint swapchainHeight;
  };

  struct BlurFilterUniformBufferObject {
    int bypassBluring;
    int i;
    int useThreeByThreeKernel;
    int ignoreLuminanceAtFirstIteration;
  };

  float mFps       = 0;

  // whether to use temporal and blur filtering
  bool mUseTemporal = true;
  bool mUseBlur     = true;

  // atrous twicking
  bool mUseThreeByThreeKernel          = true;
  bool mIgnoreLuminanceAtFirstIteration = true;

  // delta time and last recorded frame time
  float mDeltaTime = 0, mFrameRecordLastTime = 0;

  static std::unique_ptr<Camera> mCamera;
  static std::unique_ptr<Window> mWindow;

  VulkanApplicationContext *mAppContext;

  // scene for ray tracing
  std::unique_ptr<GpuModel::Scene> mRtScene;

  // compute models for ray tracing, temporal filtering, and blur filtering
  std::unique_ptr<ComputeModel> mRtxModel;
  std::unique_ptr<ComputeModel> mTemporalFilterModel;
  std::vector<std::unique_ptr<ComputeModel>> mBlurFilterPhase1Models;
  std::vector<std::unique_ptr<ComputeModel>> mBlurFilterPhase2Models;
  std::unique_ptr<ComputeModel> mBlurFilterPhase3Model;

  // buffer bundles for ray tracing, temporal filtering, and blur filtering
  std::unique_ptr<BufferBundle> mRtxBufferBundle;
  std::unique_ptr<BufferBundle> mTemperalFilterBufferBundle;
  std::vector<std::unique_ptr<BufferBundle>> mBlurFilterBufferBundles;

  std::unique_ptr<BufferBundle> mTriangleBufferBundle;
  std::unique_ptr<BufferBundle> mMaterialBufferBundle;
  std::unique_ptr<BufferBundle> mBvhBufferBundle;
  std::unique_ptr<BufferBundle> mLightsBufferBundle;

  // images for ray tracing and post-processing
  std::unique_ptr<Image> mPositionImage;
  std::unique_ptr<Image> mRawImage;
  std::unique_ptr<Image> mTargetImage;
  std::unique_ptr<Image> mAccumulationImage;
  std::unique_ptr<Image> mDepthImage;
  std::unique_ptr<Image> mNormalImage;
  std::unique_ptr<Image> mHistorySamplesImage;
  std::unique_ptr<Image> mMeshHashImage1;
  std::unique_ptr<Image> mMeshHashImage2;
  std::unique_ptr<Image> mBlurHImage;
  std::unique_ptr<Image> mATrousImage1;
  std::unique_ptr<Image> mATrousImage2;

  // command buffers for rendering and GUI
  std::vector<VkCommandBuffer> mCommandBuffers;
  std::vector<VkCommandBuffer> mGuiCommandBuffers;

  // framebuffers for GUI
  std::vector<VkFramebuffer> mGuiFrameBuffers;

  // render pass for GUI
  VkRenderPass mImGuiPass = VK_NULL_HANDLE;

  // descriptor pool for GUI
  VkDescriptorPool mGuiDescriptorPool = VK_NULL_HANDLE;

  // semaphores and fences for synchronization
  std::vector<VkSemaphore> mImageAvailableSemaphores;
  std::vector<VkSemaphore> mRenderFinishedSemaphores;
  std::vector<VkFence> mFramesInFlightFences;

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
  // initialize layouts and models
  void initScene();

  // update uniform buffer object for each frame and save last mvpe matrix
  void updateScene(uint32_t currentImage);

  // create command buffers for rendering
  void createRenderCommandBuffers();

  // create semaphores and fences for synchronization
  void createSyncObjects();

  // create command buffers for GUI
  void createGuiCommandBuffers();

  // create render pass for GUI
  void createGuiRenderPass();

  // create framebuffers for GUI
  void createGuiFramebuffers();

  // create descriptor pool for GUI
  void createGuiDescripterPool();

  // begin single time commands
  VkCommandBuffer beginSingleTimeCommands();

  // end single time commands
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  // initialize GUI
  void initGui();

  // record command buffer for GUI
  void recordGuiCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex);

  // draw a frame
  void drawFrame();

  // prepare GUI
  void prepareGui();

  // main loop
  void mainLoop();

  // initialize Vulkan
  void initVulkan();

  // cleanup resources
  void cleanup();
};