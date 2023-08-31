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
#include "render-context/RenderSystem.h"
#include "scene/ComputeMaterial.h"
#include "scene/ComputeModel.h"
#include "scene/Mesh.h"
#include "utils/Camera.h"
#include "utils/RootDir.h"
#include "utils/glm.h"
#include "utils/logger.h"

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
  };

  float mFps       = 0;
  float mFrameTime = 0;

  // whether to use temporal and blur filtering
  bool mUseTemporal = true;
  bool mUseBlur     = true;

  // delta time and last recorded frame time
  float mDeltaTime = 0, mFrameRecordLastTime = 0;

  static std::unique_ptr<Camera> mCamera;

  // scene for ray tracing
  std::shared_ptr<GpuModel::Scene> mRtScene;

  // compute models for ray tracing, temporal filtering, and blur filtering
  std::shared_ptr<ComputeModel> mRtxModel;
  std::shared_ptr<ComputeModel> mTemporalFilterModel;
  std::vector<std::shared_ptr<ComputeModel>> mBlurFilterPhase1Models;
  std::vector<std::shared_ptr<ComputeModel>> mBlurFilterPhase2Models;
  std::shared_ptr<ComputeModel> mBlurFilterPhase3Model;

  // buffer bundles for ray tracing, temporal filtering, and blur filtering
  std::shared_ptr<BufferBundle> mRtxBufferBundle;
  std::shared_ptr<BufferBundle> mTemperalFilterBufferBundle;
  std::vector<std::shared_ptr<BufferBundle>> mBlurFilterBufferBundles;

  // images for ray tracing and post-processing
  std::shared_ptr<Image> mPositionImage;
  std::shared_ptr<Image> mRawImage;
  std::shared_ptr<Image> mTargetImage;
  std::shared_ptr<Image> mAccumulationImage;
  std::shared_ptr<Image> mDepthImage;
  std::shared_ptr<Image> mNormalImage;
  std::shared_ptr<Image> mHistorySamplesImage;
  std::shared_ptr<Image> mMeshHashImage1;
  std::shared_ptr<Image> mMeshHashImage2;
  std::shared_ptr<Image> mBlurHImage;
  std::shared_ptr<Image> mATrousImage1;
  std::shared_ptr<Image> mATrousImage2;

  // command buffers for rendering and GUI
  std::vector<VkCommandBuffer> mCommandBuffers;
  std::vector<VkCommandBuffer> mGuiCommandBuffers;

  // framebuffers for GUI
  std::vector<VkFramebuffer> mSwapchainGuiFrameBuffers;

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
  static void endSingleTimeCommands(VkCommandBuffer commandBuffer);

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