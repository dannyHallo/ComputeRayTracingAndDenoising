#include "Application.h"

#include "render-context/RenderSystem.h"
#include "scene/ComputeMaterial.h"
#include "window/FullscreenWindow.h"
#include "window/HoverWindow.h"
#include "window/MaximizedWindow.h"

static const int kATrousSize        = 5;
static const int kMaxFramesInFlight = 2;
static const std::string kPathToResourceFolder =
    std::string(ROOT_DIR) + "resources/";
static const float kFpsUpdateTime = 0.5F;

std::unique_ptr<Camera> Application::mCamera = nullptr;
std::unique_ptr<Window> Application::mWindow = nullptr;

void mouseCallback(GLFWwindow * /*window*/, double xpos, double ypos) {
  static float lastX;
  static float lastY;
  static bool firstMouse = true;

  if (firstMouse) {
    lastX      = static_cast<float>(xpos);
    lastY      = static_cast<float>(ypos);
    firstMouse = false;
  }

  float xoffset = static_cast<float>(xpos) - lastX;
  float yoffset =
      lastY - static_cast<float>(
                  ypos); // reversed since y-coordinates go from bottom to top

  lastX = static_cast<float>(xpos);
  lastY = static_cast<float>(ypos);

  Application::getCamera()->processMouseMovement(xoffset, yoffset);
}

Camera *Application::getCamera() { return mCamera.get(); }

Application::Application() {
  mWindow     = std::make_unique<FullscreenWindow>();
  mAppContext = VulkanApplicationContext::initInstance(mWindow->getGlWindow());
  mCamera     = std::make_unique<Camera>(mWindow.get());
}

void Application::run() {
  initVulkan();
  mainLoop();
  cleanup();
}

void Application::cleanup() {
  Logger::print("Application is cleaning up resources...");

  for (size_t i = 0; i < kMaxFramesInFlight; i++) {
    vkDestroySemaphore(mAppContext->getDevice(), mRenderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(mAppContext->getDevice(), mImageAvailableSemaphores[i],
                       nullptr);
    vkDestroyFence(mAppContext->getDevice(), mFramesInFlightFences[i], nullptr);
  }

  for (auto &commandBuffer : mCommandBuffers) {
    vkFreeCommandBuffers(mAppContext->getDevice(),
                         mAppContext->getCommandPool(), 1, &commandBuffer);
  }

  for (auto &mGuiCommandBuffers : mGuiCommandBuffers) {
    vkFreeCommandBuffers(mAppContext->getDevice(),
                         mAppContext->getGuiCommandPool(), 1,
                         &mGuiCommandBuffers);
  }

  for (auto &guiFrameBuffer : mGuiFrameBuffers) {
    vkDestroyFramebuffer(mAppContext->getDevice(), guiFrameBuffer, nullptr);
  }

  vkDestroyRenderPass(mAppContext->getDevice(), mImGuiPass, nullptr);
  vkDestroyDescriptorPool(mAppContext->getDevice(), mGuiDescriptorPool,
                          nullptr);

  // clearing resources related to ImGui, this can be rewritten by using VMA
  // later
  ImGui_ImplVulkan_Shutdown();

  glfwTerminate();
}

void check_vk_result(VkResult resultCode) {
  Logger::checkStep("check_vk_result", resultCode);
}

void Application::initScene() {
  // equals to descriptor sets size
  auto swapchainSize = static_cast<uint32_t>(mAppContext->getSwapchainSize());

  // creates material, loads models from files, creates bvh
  mRtScene = std::make_unique<GpuModel::Scene>();

  // uniform buffers are faster to fill compared to storage buffers, but they
  // are restricted in their size Buffer bundle is an array of buffers, one per
  // each swapchain image/descriptor set.
  mRtxBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(RtxUniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  mTemperalFilterBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(TemporalFilterUniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < kATrousSize; i++) {
    auto blurFilterBufferBundle = std::make_unique<BufferBundle>(
        swapchainSize, sizeof(BlurFilterUniformBufferObject),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    mBlurFilterBufferBundles.emplace_back(std::move(blurFilterBufferBundle));
  }

  mTriangleBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Triangle) * mRtScene->triangles.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
      mRtScene->triangles.data());

  mMaterialBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Material) * mRtScene->materials.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
      mRtScene->materials.data());

  mBvhBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::BvhNode) * mRtScene->bvhNodes.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
      mRtScene->bvhNodes.data());

  mLightsBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Light) * mRtScene->lights.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
      mRtScene->lights.data());

  auto imageWidth  = mAppContext->getSwapchainExtentWidth();
  auto imageHeight = mAppContext->getSwapchainExtentHeight();

  mTargetImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mRawImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mATrousImage1 = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mATrousImage2 = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
          VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  // introducing G-Buffers
  mPositionImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mDepthImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mNormalImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mMeshHashImage1 = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mMeshHashImage2 = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mAccumulationImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  // rtx.comp
  auto rtxMat = std::make_unique<ComputeMaterial>(kPathToResourceFolder +
                                                  "/shaders/generated/rtx.spv");
  rtxMat->addUniformBufferBundle(mRtxBufferBundle.get());
  // input
  rtxMat->addStorageImage(mPositionImage.get());
  rtxMat->addStorageImage(mNormalImage.get());
  rtxMat->addStorageImage(mDepthImage.get());
  rtxMat->addStorageImage(mMeshHashImage1.get());
  // output
  rtxMat->addStorageImage(mRawImage.get());
  // buffers
  rtxMat->addStorageBufferBundle(mTriangleBufferBundle.get());
  rtxMat->addStorageBufferBundle(mMaterialBufferBundle.get());
  rtxMat->addStorageBufferBundle(mBvhBufferBundle.get());
  rtxMat->addStorageBufferBundle(mLightsBufferBundle.get());
  mRtxModel = std::make_unique<ComputeModel>(std::move(rtxMat));

  // temporalFilter.comp
  auto temporalFilterMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/temporalFilter.spv");
  temporalFilterMat->addUniformBufferBundle(mTemperalFilterBufferBundle.get());
  // input
  temporalFilterMat->addStorageImage(mPositionImage.get());
  temporalFilterMat->addStorageImage(mRawImage.get());
  temporalFilterMat->addStorageImage(mAccumulationImage.get());
  temporalFilterMat->addStorageImage(mNormalImage.get());
  temporalFilterMat->addStorageImage(mDepthImage.get());
  temporalFilterMat->addStorageImage(mMeshHashImage1.get());
  temporalFilterMat->addStorageImage(mMeshHashImage2.get());
  // output
  temporalFilterMat->addStorageImage(mATrousImage1.get());
  mTemporalFilterModel =
      std::make_unique<ComputeModel>(std::move(temporalFilterMat));

  for (int i = 0; i < kATrousSize; i++) {
    auto blurFilterPhase1Mat = std::make_unique<ComputeMaterial>(
        kPathToResourceFolder + "/shaders/generated/blurPhase1.spv");
    blurFilterPhase1Mat->addUniformBufferBundle(
        mBlurFilterBufferBundles[i].get());
    // input
    blurFilterPhase1Mat->addStorageImage(mATrousImage1.get());
    blurFilterPhase1Mat->addStorageImage(mNormalImage.get());
    blurFilterPhase1Mat->addStorageImage(mDepthImage.get());
    blurFilterPhase1Mat->addStorageImage(mPositionImage.get());
    // output
    blurFilterPhase1Mat->addStorageImage(mATrousImage2.get());
    mBlurFilterPhase1Models.emplace_back(
        std::make_unique<ComputeModel>(std::move(blurFilterPhase1Mat)));
  }

  auto blurFilterPhase3Mat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/blurPhase3.spv");
  {
    // input
    blurFilterPhase3Mat->addStorageImage(mATrousImage2.get());
    blurFilterPhase3Mat->addStorageImage(mNormalImage.get());
    blurFilterPhase3Mat->addStorageImage(mDepthImage.get());
    blurFilterPhase3Mat->addStorageImage(mPositionImage.get());
    // output
    blurFilterPhase3Mat->addStorageImage(mTargetImage.get());
  }
  mBlurFilterPhase3Model =
      std::make_unique<ComputeModel>(std::move(blurFilterPhase3Mat));
}

void Application::updateScene(uint32_t currentImage) {
  static uint32_t currentSample = 0;
  static glm::mat4 lastMvpe{1.0F};

  auto currentTime = static_cast<float>(glfwGetTime());

  RtxUniformBufferObject rtxUbo = {
      mCamera->getPosition(),
      mCamera->getFront(),
      mCamera->getUp(),
      mCamera->getRight(),
      mCamera->getVFov(),
      currentTime,
      currentSample,
      static_cast<uint32_t>(mRtScene->triangles.size()),
      static_cast<uint32_t>(mRtScene->lights.size())};

  mRtxBufferBundle->getBuffer(currentImage)->fillData(&rtxUbo);

  TemporalFilterUniformBufferObject tfUbo = {
      !mUseTemporal, lastMvpe, mAppContext->getSwapchainExtentWidth(),
      mAppContext->getSwapchainExtentHeight()};
  {
    mTemperalFilterBufferBundle->getBuffer(currentImage)->fillData(&tfUbo);

    lastMvpe =
        mCamera->getProjectionMatrix(
            static_cast<float>(mAppContext->getSwapchainExtentWidth()) /
            static_cast<float>(mAppContext->getSwapchainExtentHeight())) *
        mCamera->getViewMatrix();
  }

  for (int j = 0; j < kATrousSize; j++) {
    // update ubo for the sampleDistance
    BlurFilterUniformBufferObject bfUbo = {
        !mUseBlur,
        j,
        mICap,
        mPhiLuminance,
        mPhiDepth,
        mPhiNormal,
        mCentralKernelWeight,
        mUseThreeByThreeKernel,
        mIgnoreLuminanceAtFirstIteration,
        mChangingLuminancePhi,
    };
    mBlurFilterBufferBundles[j]->getBuffer(currentImage)->fillData(&bfUbo);
  }

  currentSample++;
}

void Application::createRenderCommandBuffers() {
  // create command buffers per swapchain image
  mCommandBuffers.resize(mAppContext->getSwapchainSize());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = mAppContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)mCommandBuffers.size();

  VkResult result = vkAllocateCommandBuffers(
      mAppContext->getDevice(), &allocInfo, mCommandBuffers.data());
  Logger::checkStep("vkAllocateCommandBuffers", result);

  VkImageMemoryBarrier targetTexGeneral2TransSrc =
      ImageUtils::generalToTransferSrcBarrier(mTargetImage->getVkImage());
  VkImageMemoryBarrier targetTexTransSrc2General =
      ImageUtils::transferSrcToGeneralBarrier(mTargetImage->getVkImage());

  VkImageMemoryBarrier accumTexGeneral2TransDst =
      ImageUtils::generalToTransferDstBarrier(mAccumulationImage->getVkImage());
  VkImageMemoryBarrier accumTexTransDst2General =
      ImageUtils::transferDstToGeneralBarrier(mAccumulationImage->getVkImage());

  VkImageMemoryBarrier aTrousTex1General2TransDst =
      ImageUtils::generalToTransferDstBarrier(mATrousImage1->getVkImage());
  VkImageMemoryBarrier aTrousTex1TransDst2General =
      ImageUtils::transferDstToGeneralBarrier(mATrousImage1->getVkImage());
  VkImageMemoryBarrier aTrousTex2General2TransSrc =
      ImageUtils::generalToTransferSrcBarrier(mATrousImage2->getVkImage());
  VkImageMemoryBarrier aTrousTex2TransSrc2General =
      ImageUtils::transferSrcToGeneralBarrier(mATrousImage2->getVkImage());

  VkImageMemoryBarrier triIdTex1General2TransSrc =
      ImageUtils::generalToTransferSrcBarrier(mMeshHashImage1->getVkImage());
  VkImageMemoryBarrier triIdTex1TransSrc2General =
      ImageUtils::transferSrcToGeneralBarrier(mMeshHashImage1->getVkImage());
  VkImageMemoryBarrier triIdTex2General2TransDst =
      ImageUtils::generalToTransferDstBarrier(mMeshHashImage2->getVkImage());
  VkImageMemoryBarrier triIdTex2TransDst2General =
      ImageUtils::transferDstToGeneralBarrier(mMeshHashImage2->getVkImage());

  VkImageCopy imgCopyRegion =
      ImageUtils::imageCopyRegion(mAppContext->getSwapchainExtentWidth(),
                                  mAppContext->getSwapchainExtentHeight());

  for (size_t i = 0; i < mCommandBuffers.size(); i++) {
    VkImageMemoryBarrier swapchainImageUndefined2TransferDst =
        ImageUtils::undefinedToTransferDstBarrier(
            mAppContext->getSwapchainImages()[i]);
    VkImageMemoryBarrier swapchainImageTransferDst2ColorAttachment =
        ImageUtils::transferDstToColorAttachmentBarrier(
            mAppContext->getSwapchainImages()[i]);

    VkCommandBuffer &currentCommandBuffer = mCommandBuffers[i];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(currentCommandBuffer, &beginInfo);
    Logger::checkStep("vkBeginCommandBuffer", result);

    VkClearColorValue clearColor{{0, 0, 0, 0}};
    VkImageSubresourceRange clearRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(currentCommandBuffer, mTargetImage->getVkImage(),
                         VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, mATrousImage1->getVkImage(),
                         VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, mATrousImage2->getVkImage(),
                         VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);

    mRtxModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                              mTargetImage->getWidth() / 32,
                              mTargetImage->getHeight() / 32, 1);

    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 0, nullptr);

    mTemporalFilterModel->computeCommand(
        currentCommandBuffer, static_cast<uint32_t>(i),
        mTargetImage->getWidth() / 32, mTargetImage->getHeight() / 32, 1);

    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 0, nullptr);

    /////////////////////////////////////////////

    for (int j = 0; j < kATrousSize; j++) {
      // update ubo for the sampleDistance
      BlurFilterUniformBufferObject bfUbo = {!mUseBlur, j};
      {
        auto &allocation =
            mBlurFilterBufferBundles[j]->getBuffer(i)->getAllocation();
        void *data;
        vmaMapMemory(mAppContext->getAllocator(), allocation, &data);
        memcpy(data, &bfUbo, sizeof(bfUbo));
        vmaUnmapMemory(mAppContext->getAllocator(), allocation);
      }

      // dispatch filter shader
      mBlurFilterPhase1Models[j]->computeCommand(
          currentCommandBuffer, static_cast<uint32_t>(i),
          mTargetImage->getWidth() / 32, mTargetImage->getHeight() / 32, 1);
      vkCmdPipelineBarrier(currentCommandBuffer,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                           0, nullptr, 0, nullptr);

      // accum texture when the first filter is done
      // (from SVGF) accumulation image should use a properly smoothed image,
      // the presented one is not ideal due to the over-blur over time
      if (j == 0) {
        vkCmdPipelineBarrier(currentCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &aTrousTex2General2TransSrc);
        vkCmdPipelineBarrier(currentCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &accumTexGeneral2TransDst);
        vkCmdPipelineBarrier(currentCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &aTrousTex1General2TransDst);
        vkCmdCopyImage(currentCommandBuffer, mATrousImage2->getVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       mATrousImage1->getVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdCopyImage(currentCommandBuffer, mATrousImage2->getVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       mAccumulationImage->getVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdPipelineBarrier(
            currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &aTrousTex1TransDst2General);
        vkCmdPipelineBarrier(
            currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &aTrousTex2TransSrc2General);
        vkCmdPipelineBarrier(currentCommandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &accumTexTransDst2General);
      }
      // copy aTrousImage2 to aTrousImage1 (excluding the last transfer)
      else if (j != kATrousSize - 1) {
        vkCmdPipelineBarrier(currentCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &aTrousTex1General2TransDst);
        vkCmdPipelineBarrier(currentCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &aTrousTex2General2TransSrc);
        vkCmdCopyImage(currentCommandBuffer, mATrousImage2->getVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       mATrousImage1->getVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdPipelineBarrier(
            currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &aTrousTex1TransDst2General);
        vkCmdPipelineBarrier(
            currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &aTrousTex2TransSrc2General);
      }
    }

    /////////////////////////////////////////////

    mBlurFilterPhase3Model->computeCommand(
        currentCommandBuffer, static_cast<uint32_t>(i),
        mTargetImage->getWidth() / 32, mTargetImage->getHeight() / 32, 1);

    // copy targetTex to swapchainImage
    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &targetTexGeneral2TransSrc);
    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &swapchainImageUndefined2TransferDst);
    vkCmdCopyImage(currentCommandBuffer, mTargetImage->getVkImage(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   mAppContext->getSwapchainImages()[i],
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &targetTexTransSrc2General);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1,
                         &swapchainImageTransferDst2ColorAttachment);

    // sync mesh hash image for next frame
    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &triIdTex1General2TransSrc);
    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &triIdTex2General2TransDst);
    vkCmdCopyImage(currentCommandBuffer, mMeshHashImage1->getVkImage(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   mMeshHashImage2->getVkImage(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &triIdTex1TransSrc2General);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &triIdTex2TransDst2General);

    // Bind graphics pipeline and dispatch draw command.
    // postProcessScene->writeRenderCommand(currentCommandBuffer, i);

    result = vkEndCommandBuffer(currentCommandBuffer);
    Logger::checkStep("vkEndCommandBuffer", result);
  }
}

void Application::createSyncObjects() {
  mImageAvailableSemaphores.resize(kMaxFramesInFlight);
  mRenderFinishedSemaphores.resize(kMaxFramesInFlight);
  mFramesInFlightFences.resize(kMaxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  // the first call to vkWaitForFences() returns immediately since the fence is
  // already signaled
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < kMaxFramesInFlight; i++) {
    if (vkCreateSemaphore(mAppContext->getDevice(), &semaphoreInfo, nullptr,
                          &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(mAppContext->getDevice(), &semaphoreInfo, nullptr,
                          &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(mAppContext->getDevice(), &fenceInfo, nullptr,
                      &mFramesInFlightFences[i]) != VK_SUCCESS) {
      Logger::throwError(
          "failed to create synchronization objects for a frame!");
    }
  }
}

void Application::createGuiCommandBuffers() {
  mGuiCommandBuffers.resize(mAppContext->getSwapchainSize());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = mAppContext->getGuiCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)mGuiCommandBuffers.size();

  VkResult result = vkAllocateCommandBuffers(
      mAppContext->getDevice(), &allocInfo, mGuiCommandBuffers.data());
  Logger::checkStep("vkAllocateCommandBuffers", result);
}

void Application::createGuiRenderPass() {
  // Imgui Pass, right after the main pass
  VkAttachmentDescription attachment = {};
  attachment.format                  = mAppContext->getSwapchainImageFormat();
  attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
  // Load onto the current render pass
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  // Store img until display time
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  // No stencil
  attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  // Present image right after this pass
  attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachment = {};
  colorAttachment.attachment            = 0;
  colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachment;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = 1;
  renderPassCreateInfo.pAttachments    = &attachment;
  renderPassCreateInfo.subpassCount    = 1;
  renderPassCreateInfo.pSubpasses      = &subpass;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies   = &dependency;

  VkResult result = vkCreateRenderPass(
      mAppContext->getDevice(), &renderPassCreateInfo, nullptr, &mImGuiPass);
  Logger::checkStep("vkCreateRenderPass", result);
}

void Application::createGuiFramebuffers() {
  // Create gui frame buffers for gui pass to use
  // Each frame buffer will have an attachment of VkImageView, in this case, the
  // attachments are mSwapchainImageViews
  mGuiFrameBuffers.resize(mAppContext->getSwapchainSize());

  // Iterate through image views
  for (size_t i = 0; i < mAppContext->getSwapchainSize(); i++) {
    VkImageView attachment = mAppContext->getSwapchainImageViews()[i];

    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass      = mImGuiPass;
    frameBufferCreateInfo.attachmentCount = 1;
    frameBufferCreateInfo.pAttachments    = &attachment;
    frameBufferCreateInfo.width  = mAppContext->getSwapchainExtentWidth();
    frameBufferCreateInfo.height = mAppContext->getSwapchainExtentHeight();
    frameBufferCreateInfo.layers = 1;

    VkResult result =
        vkCreateFramebuffer(mAppContext->getDevice(), &frameBufferCreateInfo,
                            nullptr, &mGuiFrameBuffers[i]);
    Logger::checkStep("vkCreateFramebuffer", result);
  }
}

void Application::createGuiDescripterPool() {
  VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets       = 1000 * IM_ARRAYSIZE(poolSizes);
  poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
  poolInfo.pPoolSizes    = poolSizes;

  VkResult result = vkCreateDescriptorPool(mAppContext->getDevice(), &poolInfo,
                                           nullptr, &mGuiDescriptorPool);
  Logger::checkStep("vkCreateDescriptorPool", result);
}

void Application::initGui() {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  // io.Fonts->AddFontDefault();
  io.Fonts->AddFontFromFileTTF(
      (kPathToResourceFolder + "/fonts/OverpassMono-Medium.ttf").c_str(),
      16.0f);

  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard
  // Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable
  // Gamepad Controls

  // Setup Dear ImGui style
  // ImGui::StyleColorsDark();
  ImGui::StyleColorsClassic();

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForVulkan(mWindow->getGlWindow(), true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = mAppContext->getVkInstance();
  init_info.PhysicalDevice            = mAppContext->getPhysicalDevice();
  init_info.Device                    = mAppContext->getDevice();
  init_info.QueueFamily   = mAppContext->getQueueFamilyIndices().graphicsFamily;
  init_info.Queue         = mAppContext->getGraphicsQueue();
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = mGuiDescriptorPool;
  init_info.Allocator      = VK_NULL_HANDLE;
  init_info.MinImageCount =
      static_cast<uint32_t>(mAppContext->getSwapchainSize());
  init_info.ImageCount = static_cast<uint32_t>(mAppContext->getSwapchainSize());
  init_info.CheckVkResultFn = check_vk_result;
  if (!ImGui_ImplVulkan_Init(&init_info, mImGuiPass)) {
    Logger::print("failed to init impl");
  }

  // Create fonts texture
  VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands(
      mAppContext->getDevice(), mAppContext->getCommandPool());

  if (!ImGui_ImplVulkan_CreateFontsTexture(commandBuffer)) {
    Logger::print("failed to create fonts texture");
  }
  RenderSystem::endSingleTimeCommands(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), commandBuffer);
}

void Application::recordGuiCommandBuffer(VkCommandBuffer &commandBuffer,
                                         uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags            = 0;       // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  // A call to vkBeginCommandBuffer will implicitly reset the command buffer
  VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  Logger::checkStep("vkBeginCommandBuffer", result);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass        = mImGuiPass;
  renderPassInfo.framebuffer       = mGuiFrameBuffers[imageIndex];
  renderPassInfo.renderArea.extent = mAppContext->getSwapchainExtent();

  VkClearValue clearValue{};
  clearValue.color = {{0.0F, 0.0F, 0.0F, 1.0F}};

  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues    = &clearValue;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Record Imgui Draw Data and draw funcs into command buffer
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  result = vkEndCommandBuffer(commandBuffer);
  Logger::checkStep("vkEndCommandBuffer", result);
}

void Application::drawFrame() {
  static size_t currentFrame = 0;
  vkWaitForFences(mAppContext->getDevice(), 1,
                  &mFramesInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(mAppContext->getDevice(), 1,
                &mFramesInFlightFences[currentFrame]);

  uint32_t imageIndex = 0;
  VkResult result     = vkAcquireNextImageKHR(
      mAppContext->getDevice(), mAppContext->getSwapchain(), UINT64_MAX,
      mImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    // sub-optimal: a swapchain no longer matches the surface properties
    // exactly, but can still be used to present to the surface successfully
    Logger::throwError("resizing is not allowed!");
  }

  updateScene(imageIndex);

  recordGuiCommandBuffer(mGuiCommandBuffers[imageIndex], imageIndex);
  std::array<VkCommandBuffer, 2> submitCommandBuffers = {
      mCommandBuffers[imageIndex], mGuiCommandBuffers[imageIndex]};

  VkSubmitInfo submitInfo{};
  {
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // wait until the image is ready
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = &mImageAvailableSemaphores[currentFrame];
    // signal a semaphore after render finished
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &mRenderFinishedSemaphores[currentFrame];

    // wait for no stage
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    submitInfo.pWaitDstStageMask =
        static_cast<VkPipelineStageFlags *>(waitStages);

    submitInfo.commandBufferCount =
        static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers = submitCommandBuffers.data();
  }

  result = vkQueueSubmit(mAppContext->getGraphicsQueue(), 1, &submitInfo,
                         mFramesInFlightFences[currentFrame]);
  Logger::checkStep("vkQueueSubmit", result);

  VkPresentInfoKHR presentInfo{};
  {
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &mRenderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &mAppContext->getSwapchain();
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;
  }

  result = vkQueuePresentKHR(mAppContext->getPresentQueue(), &presentInfo);
  Logger::checkStep("vkQueuePresentKHR", result);

  // Commented this out for playing around with it later :)
  // vkQueueWaitIdle(context.getPresentQueue());
  currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
}

void Application::prepareGui() {
  ImGui_ImplVulkan_NewFrame();

  // handle the user inputs, the screen resize
  ImGui_ImplGlfw_NewFrame();

  ImGui::NewFrame();

  ImGui::BeginMainMenuBar();
  ImGui::SetWindowFontScale(1.2F);
  if (ImGui::BeginMenu("Config")) {
    ImGui::SliderInt("A-Trous times", &mICap, 0, 5);
    ImGui::SliderFloat("Luminance Phi", &mPhiLuminance, 0.0F, 1.0F);
    ImGui::SliderFloat("Phi Depth", &mPhiDepth, 0.0F, 1.0F);
    ImGui::SliderFloat("Phi Normal", &mPhiNormal, 0.0F, 500.0F);
    ImGui::SliderFloat("Central Kernel Weight", &mCentralKernelWeight, 0.0F,
                       1.0F);
    ImGui::Checkbox("Temporal Accumulation", &mUseTemporal);
    ImGui::Checkbox("Use 3x3 A-Trous", &mUseThreeByThreeKernel);
    ImGui::Checkbox("Ignore Luminance For First Iteration",
                    &mIgnoreLuminanceAtFirstIteration);
    ImGui::Checkbox("Changing luminance phi", &mChangingLuminancePhi);
    ImGui::Checkbox("A-Trous", &mUseBlur);
    ImGui::EndMenu();
  }
  ImGui::EndMainMenuBar();

  auto *mainGuiViewPort = ImGui::GetMainViewport();
  // float width           = mainGuiViewPort->Size.x;
  float height = mainGuiViewPort->Size.y;

  const float kStatsWindowWidth  = 100.0F;
  const float kStatsWindowHeight = 120.0F;

  ImGui::SetNextWindowPos(ImVec2(0, height - kStatsWindowHeight));
  ImGui::Begin(
      "Stats", nullptr,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
          ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
          ImGuiWindowFlags_NoFocusOnAppearing |
          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

  ImGui::SetWindowSize(ImVec2(kStatsWindowWidth, kStatsWindowHeight));
  ImGui::SetWindowFontScale(1.2F);
  ImGui::Text("fps : %s", (std::to_string(static_cast<int>(mFps))).c_str());
  ImGui::Text("mspf: %s",
              (std::to_string(static_cast<int>(1 / mFps * 1000.F))).c_str());

  ImGui::End();
  ImGui::Render();
}

void Application::mainLoop() {
  static float fpsFrameCount     = 0;
  static float fpsRecordLastTime = 0;

  while (glfwWindowShouldClose(mWindow->getGlWindow()) == 0) {
    glfwPollEvents();

    prepareGui();

    fpsFrameCount++;
    auto currentTime     = static_cast<float>(glfwGetTime());
    mDeltaTime           = currentTime - mFrameRecordLastTime;
    mFrameRecordLastTime = currentTime;

    if (currentTime - fpsRecordLastTime >= kFpsUpdateTime) {
      mFps              = fpsFrameCount / kFpsUpdateTime;
      std::string title = "FPS - " + std::to_string(static_cast<int>(mFps));
      glfwSetWindowTitle(mWindow->getGlWindow(), title.c_str());
      fpsFrameCount = 0;

      fpsRecordLastTime = currentTime;
    }

    mCamera->processInput(mDeltaTime);
    drawFrame();
  }

  vkDeviceWaitIdle(mAppContext->getDevice());
}

void Application::initVulkan() {

  // initialize the scene
  initScene();

  // create render command buffers
  createRenderCommandBuffers();

  // create GUI command buffers
  createGuiCommandBuffers();

  // create synchronization objects
  createSyncObjects();

  // create GUI render pass
  createGuiRenderPass();

  // create GUI framebuffers
  createGuiFramebuffers();

  // create GUI descriptor pool
  createGuiDescripterPool();

  // initialize GUI
  initGui();

  // set mouse callback function to be called whenever the cursor position
  // changes
  glfwSetCursorPosCallback(mWindow->getGlWindow(), mouseCallback);
}