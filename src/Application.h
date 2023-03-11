#pragma once

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
#include "utils/vulkan.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <vector>

class Application {
public:
  Application();
  void run();

private:
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

  // bigger phi indicates bigger tolerence to apply blur
  struct BlurFilterUniformBufferObject {
    int bypassBluring; // just a 4 bytes bool
    int i;
  };

  const int aTrousSize           = 5;

  float fps                      = 0;
  float frameTime                = 0;
  
  bool useTemporal               = true;
  bool useBlur                   = true;
  const int MAX_FRAMES_IN_FLIGHT = 2;
  const std::string path_prefix  = std::string(ROOT_DIR) + "resources/";
  const float fpsUpdateTime      = 0.5f;
  float deltaTime = 0, frameRecordLastTime = 0;

  std::shared_ptr<GpuModel::Scene> rtScene;

  std::shared_ptr<ComputeModel> rtxModel;
  std::shared_ptr<ComputeModel> temporalFilterModel;
  std::vector<std::shared_ptr<ComputeModel>> blurFilterPhase1Models;
  std::vector<std::shared_ptr<ComputeModel>> blurFilterPhase2Models;
  std::shared_ptr<ComputeModel> blurFilterPhase3Model;

  std::shared_ptr<Scene> postProcessScene;

  std::shared_ptr<BufferBundle> rtxBufferBundle;
  std::shared_ptr<BufferBundle> temperalFilterBufferBundle;
  std::vector<std::shared_ptr<BufferBundle>> blurFilterBufferBundles;

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

  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkCommandBuffer> guiCommandBuffers;
  std::vector<VkFramebuffer> swapchainGuiFrameBuffers;

  VkRenderPass imGuiPass;
  VkDescriptorPool guiDescriptorPool;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> framesInFlightFences;

  // initialize layouts and models.
  void initScene();

  // updates ubo for each frame, and saves the last mvpe matrix
  void updateScene(uint32_t currentImage);

  void createRenderCommandBuffers();

  void createSyncObjects();

  void createGuiCommandBuffers();

  void createGuiRenderPass();

  void createGuiFramebuffers();

  void createGuiDescripterPool();

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  void initGui();

  void recordGuiCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex);

  void drawFrame();

  void prepareGui();
  void mainLoop();
  void initVulkan();

  void cleanup();
};