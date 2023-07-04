#pragma once

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <vector>

#include "app-context/VulkanApplicationContext.h"

#include "memory/ImageUtils.h"
#include "ray-tracing/RtScene.h"
#include "render-context/RenderSystem.h"
#include "scene/ComputeMaterial.h"
#include "scene/ComputeModel.h"
#include "scene/DrawableModel.h"
#include "scene/Mesh.h"
#include "scene/Scene.h"
#include "utils/Camera.h"
#include "utils/RootDir.h"
#include "utils/glm.h"
#include "utils/systemLog.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

class Application {
  struct RtxUniformBufferObject {
    alignas(16) glm::vec3 camPosition;
    alignas(16) glm::vec3 camFront;
    alignas(16) glm::vec3 camUp;
    alignas(16) glm::vec3 camRight;
    float vfov;
    float time;
    uint32_t currentSample;
    uint32_t numTriangles;
    uint32_t numLights;
    uint32_t numSpheres;
  };

  struct TemporalFilterUniformBufferObject {
    int bypassTemporalFiltering;
    alignas(16) glm::mat4 lastMvpe;
    uint swapchainWidth;
    uint swapchainHeight;
  };

  struct BlurFilterUniformBufferObject {
    int bypassBluring;
    int i;
  };

  const int aTrousSize = 5;

  float fps       = 0;
  float frameTime = 0;

  // whether to use temporal and blur filtering
  bool useTemporal = true;
  bool useBlur     = true;

  const int maxFramesInFlight            = 2;
  const std::string pathToResourceFolder = std::string(ROOT_DIR) + "resources/";
  const float fpsUpdateTime              = 0.5f;

  // delta time and last recorded frame time
  float deltaTime = 0, frameRecordLastTime = 0;

  // scene for ray tracing
  std::shared_ptr<GpuModel::Scene> rtScene;

  // compute models for ray tracing, temporal filtering, and blur filtering
  std::shared_ptr<ComputeModel> rtxModel;
  std::shared_ptr<ComputeModel> temporalFilterModel;
  std::vector<std::shared_ptr<ComputeModel>> blurFilterPhase1Models;
  std::vector<std::shared_ptr<ComputeModel>> blurFilterPhase2Models;
  std::shared_ptr<ComputeModel> blurFilterPhase3Model;

  // scene for post-processing
  std::shared_ptr<Scene> postProcessScene;

  // buffer bundles for ray tracing, temporal filtering, and blur filtering
  std::shared_ptr<BufferBundle> rtxBufferBundle;
  std::shared_ptr<BufferBundle> temperalFilterBufferBundle;
  std::vector<std::shared_ptr<BufferBundle>> blurFilterBufferBundles;

  // images for ray tracing and post-processing
  std::shared_ptr<Image> positionImage;
  std::shared_ptr<Image> rawImage;
  std::shared_ptr<Image> targetImage;
  std::shared_ptr<Image> accumulationImage;
  std::shared_ptr<Image> depthImage;
  std::shared_ptr<Image> normalImage;
  std::shared_ptr<Image> historySamplesImage;
  std::shared_ptr<Image> meshHashImage1;
  std::shared_ptr<Image> meshHashImage2;
  std::shared_ptr<Image> blurHImage;
  std::shared_ptr<Image> aTrousImage1;
  std::shared_ptr<Image> aTrousImage2;

  // command buffers for rendering and GUI
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkCommandBuffer> guiCommandBuffers;

  // framebuffers for GUI
  std::vector<VkFramebuffer> swapchainGuiFrameBuffers;

  // render pass for GUI
  VkRenderPass imGuiPass;

  // descriptor pool for GUI
  VkDescriptorPool guiDescriptorPool;

  // semaphores and fences for synchronization
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> framesInFlightFences;

public:
  Application() = default;
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