#include "app-context/VulkanApplicationContext.h"
#include "app-context/VulkanGlobal.h"
#include "app-context/VulkanSwapchain.h"
#include "memory/ImageUtils.h"
#include "ray-tracing/RtScene.h"
// #include "render-context/FlatRenderPass.h"
#include "render-context/RenderSystem.h"
#include "scene/ComputeMaterial.h"
#include "scene/ComputeModel.h"
#include "scene/DrawableModel.h"
#include "scene/Mesh.h"
#include "scene/Scene.h"
#include "utils/Camera.h"
#include "utils/RootDir.h"
#include "utils/glm.h"
#include "utils/vulkan.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <vector>

using namespace std;

const std::string path_prefix = std::string(ROOT_DIR) + "resources/";

float mouseOffsetX, mouseOffsetY;
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);

// Camera camera(glm::vec3(1.8f, 8.6f, 1.1f));

Camera camera{};

const float fpsUpdateTime = 0.5f;

bool hasMoved   = false;
float deltaTime = 0, frameRecordLastTime = 0;

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
  float swapchainWidth;
  float swapchainHeight;
};

// bigger phi indicates bigger tolerence to apply blur
struct BlurFilterUniformBufferObject {
  int bypassBluring; // just a 4 bytes bool
  float sigma;
  float cPhi;
  float nPhi;
  float pPhi;
  int i;
};

class HelloComputeApplication {
public:
  void run() {
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  std::shared_ptr<GpuModel::Scene> rtScene;

  std::shared_ptr<mcvkp::ComputeModel> rtxModel;
  std::shared_ptr<mcvkp::ComputeModel> temporalFilterModel;
  std::vector<std::shared_ptr<mcvkp::ComputeModel>> blurFilterPhase1Models;
  std::vector<std::shared_ptr<mcvkp::ComputeModel>> blurFilterPhase2Models;

  std::shared_ptr<mcvkp::Scene> postProcessScene;

  std::shared_ptr<mcvkp::BufferBundle> rtxBufferBundle;
  std::shared_ptr<mcvkp::BufferBundle> temperalFilterBufferBundle;
  std::vector<std::shared_ptr<mcvkp::BufferBundle>> blurFilterBufferBundles;

  std::shared_ptr<mcvkp::Image> positionImage;
  std::shared_ptr<mcvkp::Image> rawImage;
  std::shared_ptr<mcvkp::Image> targetImage;
  std::shared_ptr<mcvkp::Image> accumulationImage;
  std::shared_ptr<mcvkp::Image> normalImage;
  std::shared_ptr<mcvkp::Image> triIdImage1;
  std::shared_ptr<mcvkp::Image> triIdImage2;
  std::shared_ptr<mcvkp::Image> blurHImage;
  std::shared_ptr<mcvkp::Image> aTrousImage1;
  std::shared_ptr<mcvkp::Image> aTrousImage2;

  std::vector<VkCommandBuffer> commandBuffers;

  const int MAX_FRAMES_IN_FLIGHT = 2;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> framesInFlightFences;

  // Initializing layouts and models.
  void initScene() {
    using namespace mcvkp;
    // = descriptor sets size
    uint32_t swapchainImageViewSize = static_cast<uint32_t>(VulkanGlobal::swapchainContext.getImageViews().size());

    rtScene = std::make_shared<GpuModel::Scene>();

    // uniform buffers are faster to be filled compared to storage buffers, but they are restricted in their size

    // Buffer bundle is an array of buffers, one per each swapchain image/descriptor set.
    rtxBufferBundle = std::make_shared<mcvkp::BufferBundle>(swapchainImageViewSize);
    BufferUtils::createBundle<RtxUniformBufferObject>(rtxBufferBundle.get(), RtxUniformBufferObject{},
                                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    temperalFilterBufferBundle = std::make_shared<mcvkp::BufferBundle>(swapchainImageViewSize);
    BufferUtils::createBundle<TemporalFilterUniformBufferObject>(temperalFilterBufferBundle.get(),
                                                                 TemporalFilterUniformBufferObject{},
                                                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    for (int i = 0; i < 5; i++) {
      auto blurFilterBufferBundle = std::make_shared<mcvkp::BufferBundle>(swapchainImageViewSize);
      BufferUtils::createBundle<BlurFilterUniformBufferObject>(blurFilterBufferBundle.get(), BlurFilterUniformBufferObject{},
                                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
      blurFilterBufferBundles.emplace_back(blurFilterBufferBundle);
    }

    auto triangleBufferBundle = std::make_shared<mcvkp::BufferBundle>(swapchainImageViewSize);
    BufferUtils::createBundle<GpuModel::Triangle>(triangleBufferBundle.get(), rtScene->triangles.data(),
                                                  rtScene->triangles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);

    auto materialBufferBundle = std::make_shared<mcvkp::BufferBundle>(swapchainImageViewSize);
    BufferUtils::createBundle<GpuModel::Material>(materialBufferBundle.get(), rtScene->materials.data(),
                                                  rtScene->materials.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);

    auto aabbBufferBundle = std::make_shared<mcvkp::BufferBundle>(swapchainImageViewSize);
    BufferUtils::createBundle<GpuModel::BvhNode>(aabbBufferBundle.get(), rtScene->bvhNodes.data(), rtScene->bvhNodes.size(),
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    auto lightsBufferBundle = std::make_shared<mcvkp::BufferBundle>(swapchainImageViewSize);
    BufferUtils::createBundle<GpuModel::Light>(lightsBufferBundle.get(), rtScene->lights.data(), rtScene->lights.size(),
                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    auto spheresBufferBundle = std::make_shared<mcvkp::BufferBundle>(swapchainImageViewSize);
    BufferUtils::createBundle<GpuModel::Sphere>(spheresBufferBundle.get(), rtScene->spheres.data(), rtScene->spheres.size(),
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    targetImage = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                   VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, targetImage);
    mcvkp::ImageUtils::transitionImageLayout(targetImage->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    rawImage = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT,
                                   VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, rawImage);
    mcvkp::ImageUtils::transitionImageLayout(rawImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    blurHImage = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                   VMA_MEMORY_USAGE_GPU_ONLY, blurHImage);
    mcvkp::ImageUtils::transitionImageLayout(blurHImage->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    aTrousImage1 = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                   VMA_MEMORY_USAGE_GPU_ONLY, aTrousImage1);
    mcvkp::ImageUtils::transitionImageLayout(aTrousImage1->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    aTrousImage2 = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                   VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, aTrousImage2);
    mcvkp::ImageUtils::transitionImageLayout(aTrousImage2->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    positionImage = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                   VMA_MEMORY_USAGE_GPU_ONLY, positionImage);
    mcvkp::ImageUtils::transitionImageLayout(positionImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    normalImage = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                   VMA_MEMORY_USAGE_GPU_ONLY, normalImage);
    mcvkp::ImageUtils::transitionImageLayout(normalImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    triIdImage1 = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R32_SINT, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                   VMA_MEMORY_USAGE_GPU_ONLY, triIdImage1);
    mcvkp::ImageUtils::transitionImageLayout(triIdImage1->image, VK_FORMAT_R32_SINT, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    triIdImage2 = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R32_SINT, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                   VMA_MEMORY_USAGE_GPU_ONLY, triIdImage2);
    mcvkp::ImageUtils::transitionImageLayout(triIdImage2->image, VK_FORMAT_R32_SINT, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    accumulationImage = std::make_shared<mcvkp::Image>();
    mcvkp::ImageUtils::createImage(VulkanGlobal::swapchainContext.getExtent().width,
                                   VulkanGlobal::swapchainContext.getExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                                   VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                   VMA_MEMORY_USAGE_GPU_ONLY, accumulationImage);
    mcvkp::ImageUtils::transitionImageLayout(accumulationImage->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL, 1);

    // rtx.comp
    auto rtxMat = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/rtx.spv");
    {
      rtxMat->addUniformBufferBundle(rtxBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
      // input
      rtxMat->addStorageImage(positionImage, VK_SHADER_STAGE_COMPUTE_BIT);
      rtxMat->addStorageImage(normalImage, VK_SHADER_STAGE_COMPUTE_BIT);
      rtxMat->addStorageImage(triIdImage1, VK_SHADER_STAGE_COMPUTE_BIT);
      // output
      rtxMat->addStorageImage(rawImage, VK_SHADER_STAGE_COMPUTE_BIT);
      rtxMat->addStorageBufferBundle(triangleBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
      rtxMat->addStorageBufferBundle(materialBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
      rtxMat->addStorageBufferBundle(aabbBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
      rtxMat->addStorageBufferBundle(lightsBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
      rtxMat->addStorageBufferBundle(spheresBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    rtxModel = std::make_shared<ComputeModel>(rtxMat);

    // temporalFilter.comp
    auto temporalFilterMat = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/temporalFilter.spv");
    {
      temporalFilterMat->addUniformBufferBundle(temperalFilterBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
      // input
      temporalFilterMat->addStorageImage(positionImage, VK_SHADER_STAGE_COMPUTE_BIT);
      temporalFilterMat->addStorageImage(rawImage, VK_SHADER_STAGE_COMPUTE_BIT);
      temporalFilterMat->addStorageImage(accumulationImage, VK_SHADER_STAGE_COMPUTE_BIT);
      temporalFilterMat->addStorageImage(normalImage, VK_SHADER_STAGE_COMPUTE_BIT);
      temporalFilterMat->addStorageImage(triIdImage1, VK_SHADER_STAGE_COMPUTE_BIT);
      temporalFilterMat->addStorageImage(triIdImage2, VK_SHADER_STAGE_COMPUTE_BIT);
      // output
      temporalFilterMat->addStorageImage(aTrousImage1, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    temporalFilterModel = std::make_shared<ComputeModel>(temporalFilterMat);

    for (int i = 0; i < 5; i++) {
      auto blurFilterPhase1Mat = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/blurPhase1.spv");
      {
        blurFilterPhase1Mat->addUniformBufferBundle(blurFilterBufferBundles[i], VK_SHADER_STAGE_COMPUTE_BIT);
        // input
        blurFilterPhase1Mat->addStorageImage(aTrousImage1, VK_SHADER_STAGE_COMPUTE_BIT);
        blurFilterPhase1Mat->addStorageImage(normalImage, VK_SHADER_STAGE_COMPUTE_BIT);
        blurFilterPhase1Mat->addStorageImage(positionImage, VK_SHADER_STAGE_COMPUTE_BIT);
        // output
        blurFilterPhase1Mat->addStorageImage(blurHImage, VK_SHADER_STAGE_COMPUTE_BIT);
      }
      blurFilterPhase1Models.emplace_back(std::make_shared<ComputeModel>(blurFilterPhase1Mat));
    }

    for (int i = 0; i < 5; i++) {
      auto blurFilterPhase2Mat = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/blurPhase2.spv");
      {
        blurFilterPhase2Mat->addUniformBufferBundle(blurFilterBufferBundles[i], VK_SHADER_STAGE_COMPUTE_BIT);
        // input
        blurFilterPhase2Mat->addStorageImage(aTrousImage1, VK_SHADER_STAGE_COMPUTE_BIT);
        blurFilterPhase2Mat->addStorageImage(blurHImage, VK_SHADER_STAGE_COMPUTE_BIT);
        blurFilterPhase2Mat->addStorageImage(normalImage, VK_SHADER_STAGE_COMPUTE_BIT);
        blurFilterPhase2Mat->addStorageImage(positionImage, VK_SHADER_STAGE_COMPUTE_BIT);
        // output
        blurFilterPhase2Mat->addStorageImage(aTrousImage2, VK_SHADER_STAGE_COMPUTE_BIT);
      }
      blurFilterPhase2Models.emplace_back(std::make_shared<ComputeModel>(blurFilterPhase2Mat));
    }

    postProcessScene = std::make_shared<Scene>(RenderPassType::eFlat);

    auto screenTex      = std::make_shared<Texture>(targetImage);
    auto screenMaterial = std::make_shared<Material>(path_prefix + "/shaders/generated/post-process-vert.spv",
                                                     path_prefix + "/shaders/generated/post-process-frag.spv");
    screenMaterial->addTexture(screenTex, VK_SHADER_STAGE_FRAGMENT_BIT);
    postProcessScene->addModel(std::make_shared<DrawableModel>(screenMaterial, MeshType::ePlane));
  }

  void updateScene(uint32_t currentImage) {
    static uint32_t currentSample = 0;
    static glm::mat4 lastMvpe{1.0f};

    float currentTime = (float)glfwGetTime();

    if (hasMoved) {
      // currentSample = 0;
      hasMoved = false;
    }

    RtxUniformBufferObject rtxUbo = {camera.position,
                                     camera.front,
                                     camera.up,
                                     camera.right,
                                     camera.vFov,
                                     currentTime,
                                     currentSample,
                                     (uint32_t)rtScene->triangles.size(),
                                     (uint32_t)rtScene->lights.size(),
                                     (uint32_t)rtScene->spheres.size()};
    {
      auto &allocation = rtxBufferBundle->buffers[currentImage]->allocation;
      void *data;
      vmaMapMemory(VulkanGlobal::context.getAllocator(), allocation, &data);
      memcpy(data, &rtxUbo, sizeof(rtxUbo));
      vmaUnmapMemory(VulkanGlobal::context.getAllocator(), allocation);
    }

    TemporalFilterUniformBufferObject tfUbo = {false, lastMvpe, VulkanGlobal::swapchainContext.getExtent().width,
                                               VulkanGlobal::swapchainContext.getExtent().height};
    {
      auto &allocation = temperalFilterBufferBundle->buffers[currentImage]->allocation;
      void *data;
      vmaMapMemory(VulkanGlobal::context.getAllocator(), allocation, &data);
      memcpy(data, &tfUbo, sizeof(tfUbo));
      vmaUnmapMemory(VulkanGlobal::context.getAllocator(), allocation);

      lastMvpe = camera.getProjectionMatrix(static_cast<float>(VulkanGlobal::swapchainContext.getExtent().width) /
                                            static_cast<float>(VulkanGlobal::swapchainContext.getExtent().height)) *
                 camera.getViewMatrix();
    }

    currentSample++;
  }

  void createCommandBuffers() {
    commandBuffers.resize(VulkanGlobal::swapchainContext.getImageViews().size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = VulkanGlobal::context.getCommandPool();
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(VulkanGlobal::context.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }

    VkImageMemoryBarrier targetTexTransDst2ReadOnly = mcvkp::ImageUtils::transferDstToReadOnlyBarrier(targetImage->image);
    VkImageMemoryBarrier targetTexReadOnly2General  = mcvkp::ImageUtils::readOnlyToGeneralBarrier(targetImage->image);
    VkImageMemoryBarrier targetTexGeneral2TransDst  = mcvkp::ImageUtils::generalToTransferDstBarrier(targetImage->image);
    VkImageMemoryBarrier targetTexTransDst2General  = mcvkp::ImageUtils::transferDstToGeneralBarrier(targetImage->image);

    VkImageMemoryBarrier accumTexGeneral2TransDst = mcvkp::ImageUtils::generalToTransferDstBarrier(accumulationImage->image);
    VkImageMemoryBarrier accumTexTransDst2General = mcvkp::ImageUtils::transferDstToGeneralBarrier(accumulationImage->image);

    VkImageMemoryBarrier aTrousTex1General2TransDst = mcvkp::ImageUtils::generalToTransferDstBarrier(aTrousImage1->image);
    VkImageMemoryBarrier aTrousTex1TransDst2General = mcvkp::ImageUtils::transferDstToGeneralBarrier(aTrousImage1->image);
    VkImageMemoryBarrier aTrousTex2General2TransSrc = mcvkp::ImageUtils::generalToTransferSrcBarrier(aTrousImage2->image);
    VkImageMemoryBarrier aTrousTex2TransSrc2General = mcvkp::ImageUtils::transferSrcToGeneralBarrier(aTrousImage2->image);

    VkImageMemoryBarrier triIdTex1General2TransSrc = mcvkp::ImageUtils::generalToTransferSrcBarrier(triIdImage1->image);
    VkImageMemoryBarrier triIdTex1TransSrc2General = mcvkp::ImageUtils::transferSrcToGeneralBarrier(triIdImage1->image);
    VkImageMemoryBarrier triIdTex2General2TransDst = mcvkp::ImageUtils::generalToTransferDstBarrier(triIdImage2->image);
    VkImageMemoryBarrier triIdTex2TransDst2General = mcvkp::ImageUtils::transferDstToGeneralBarrier(triIdImage2->image);

    VkImageCopy imgCopyRegion = mcvkp::ImageUtils::imageCopyRegion(VulkanGlobal::swapchainContext.getExtent().width,
                                                                   VulkanGlobal::swapchainContext.getExtent().height);

    for (size_t i = 0; i < commandBuffers.size(); i++) {
      VkCommandBuffer &currentCommandBuffer = commandBuffers[i];

      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags            = 0;       // Optional
      beginInfo.pInheritanceInfo = nullptr; // Optional

      if (vkBeginCommandBuffer(currentCommandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
      }

      VkClearColorValue clearColor{0, 0, 0, 0};
      VkImageSubresourceRange clearRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      vkCmdClearColorImage(currentCommandBuffer, positionImage->image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
      vkCmdClearColorImage(currentCommandBuffer, targetImage->image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
      vkCmdClearColorImage(currentCommandBuffer, normalImage->image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
      vkCmdClearColorImage(currentCommandBuffer, aTrousImage1->image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
      vkCmdClearColorImage(currentCommandBuffer, aTrousImage2->image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
      vkCmdClearColorImage(currentCommandBuffer, blurHImage->image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);

      rtxModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->width / 32, targetImage->height / 32,
                               1);

      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                           nullptr, 0, nullptr, 0, nullptr);

      temporalFilterModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->width / 32,
                                          targetImage->height / 32, 1);

      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                           nullptr, 0, nullptr, 0, nullptr);

      /////////////////////////////////////////////

      for (int j = 0; j < 5; j++) {
        // update ubo for the sampleDistance
        BlurFilterUniformBufferObject bfUbo = {false, 4, 0.8, 0.6, 0.1, j};
        {
          auto &allocation = blurFilterBufferBundles[j]->buffers[i]->allocation;
          void *data;
          vmaMapMemory(VulkanGlobal::context.getAllocator(), allocation, &data);
          memcpy(data, &bfUbo, sizeof(bfUbo));
          vmaUnmapMemory(VulkanGlobal::context.getAllocator(), allocation);
        }

        // dispatch 2 stages of separate shaders
        blurFilterPhase1Models[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->width / 32,
                                                  targetImage->height / 32, 1);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 0, nullptr);
        blurFilterPhase2Models[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->width / 32,
                                                  targetImage->height / 32, 1);

        // copy aTrousImage2 to aTrousImage1
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &aTrousTex1General2TransDst);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &aTrousTex2General2TransSrc);
        vkCmdCopyImage(currentCommandBuffer, aTrousImage2->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, aTrousImage1->image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &aTrousTex1TransDst2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &aTrousTex2TransSrc2General);
      }
      /////////////////////////////////////////////

      // copy aTrousImage2 to targetTex
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &aTrousTex2General2TransSrc);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &targetTexGeneral2TransDst);
      vkCmdCopyImage(currentCommandBuffer, aTrousImage2->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetImage->image,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &targetTexTransDst2ReadOnly);

      // copy aTrousImage2 to accumTex
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &accumTexGeneral2TransDst);
      vkCmdCopyImage(currentCommandBuffer, aTrousImage2->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, accumulationImage->image,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &aTrousTex2TransSrc2General);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &accumTexTransDst2General);

      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &triIdTex1General2TransSrc);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &triIdTex2General2TransDst);
      vkCmdCopyImage(currentCommandBuffer, triIdImage1->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, triIdImage2->image,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &triIdTex1TransSrc2General);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                           nullptr, 0, nullptr, 1, &triIdTex2TransDst2General);

      // Bind graphics pipeline and dispatch draw command.
      postProcessScene->writeRenderCommand(currentCommandBuffer, i);

      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                           0, nullptr, 0, nullptr, 1, &targetTexReadOnly2General);

      if (vkEndCommandBuffer(currentCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
      }
    }
  }

  void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    framesInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    // the first call to vkWaitForFences() returns immediately since the fence is already signaled
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (vkCreateSemaphore(VulkanGlobal::context.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
              VK_SUCCESS ||
          vkCreateSemaphore(VulkanGlobal::context.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
              VK_SUCCESS ||
          vkCreateFence(VulkanGlobal::context.getDevice(), &fenceInfo, nullptr, &framesInFlightFences[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
      }
    }
  }

  void drawFrame() {
    static size_t currentFrame = 0;
    vkWaitForFences(VulkanGlobal::context.getDevice(), 1, &framesInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(VulkanGlobal::context.getDevice(), 1, &framesInFlightFences[currentFrame]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(VulkanGlobal::context.getDevice(), VulkanGlobal::swapchainContext.getBody(),
                                            UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      // sub-optimal: a swapchain no longer matches the surface properties exactly, but can still be used to present
      // to the surface successfully
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateScene(imageIndex);

    VkSubmitInfo submitInfo{};
    {
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      // wait until the image is ready
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores    = &imageAvailableSemaphores[currentFrame];
      // signal a semaphore after render finished
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores    = &renderFinishedSemaphores[currentFrame];

      // wait for no stage
      VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
      submitInfo.pWaitDstStageMask      = waitStages;

      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers    = &commandBuffers[imageIndex];
    }

    if (vkQueueSubmit(VulkanGlobal::context.getGraphicsQueue(), 1, &submitInfo, framesInFlightFences[currentFrame]) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    {
      presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores    = &renderFinishedSemaphores[currentFrame];
      presentInfo.swapchainCount     = 1;
      presentInfo.pSwapchains        = &VulkanGlobal::swapchainContext.getBody();
      presentInfo.pImageIndices      = &imageIndex;
      presentInfo.pResults           = nullptr;
    }

    result = vkQueuePresentKHR(VulkanGlobal::context.getPresentQueue(), &presentInfo);

    if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }

    // Commented this out for playing around with it later :)
    // vkQueueWaitIdle(VulkanGlobal::context.getPresentQueue());
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void mainLoop() {
    static float fpsFrameCount = 0, fpsRecordLastTime = 0;

    while (!glfwWindowShouldClose(VulkanGlobal::context.getWindow())) {
      fpsFrameCount++;
      float currentTime   = static_cast<float>(glfwGetTime());
      deltaTime           = currentTime - frameRecordLastTime;
      frameRecordLastTime = currentTime;

      if (currentTime - fpsRecordLastTime >= fpsUpdateTime) {
        string title = "FPS - " + to_string(static_cast<int>(fpsFrameCount / fpsUpdateTime));
        glfwSetWindowTitle(VulkanGlobal::context.getWindow(), title.c_str());
        fpsFrameCount = 0;

        fpsRecordLastTime = currentTime;
      }

      processInput(VulkanGlobal::context.getWindow());
      glfwPollEvents();
      drawFrame();
    }

    vkDeviceWaitIdle(VulkanGlobal::context.getDevice());
  }

  void initVulkan() {
    initScene();
    createCommandBuffers();
    createSyncObjects();
    glfwSetCursorPosCallback(VulkanGlobal::context.getWindow(), mouse_callback);
  }

  void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(VulkanGlobal::context.getDevice(), renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(VulkanGlobal::context.getDevice(), imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(VulkanGlobal::context.getDevice(), framesInFlightFences[i], nullptr);
    }

    glfwTerminate();
  }
};

int main() {
  HelloComputeApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void processInput(GLFWwindow *window) {
  Camera_Movement direction = NONE;
  hasMoved                  = false;
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    direction = UP;
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    direction = DOWN;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    direction = FORWARD;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    direction = BACKWARD;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    direction = LEFT;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    direction = RIGHT;

  if (direction != NONE) {
    camera.processKeyboard(direction, deltaTime);
    hasMoved = true;
  }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  static float lastX, lastY;
  static bool firstMouse = true;

  hasMoved = true;

  if (firstMouse) {
    lastX      = static_cast<float>(xpos);
    lastY      = static_cast<float>(ypos);
    firstMouse = false;
  }

  float xoffset = static_cast<float>(xpos) - lastX;
  float yoffset = lastY - static_cast<float>(ypos); // reversed since y-coordinates go from bottom to top

  lastX = static_cast<float>(xpos);
  lastY = static_cast<float>(ypos);

  camera.processMouseMovement(xoffset, yoffset);
}
