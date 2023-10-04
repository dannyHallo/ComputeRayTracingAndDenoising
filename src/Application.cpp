#include "Application.h"

#include "render-context/RenderSystem.h"
#include "scene/ComputeMaterial.h"
#include "window/Window.h"

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
  mWindow     = std::make_unique<Window>(WindowStyle::kFullScreen);
  mAppContext = VulkanApplicationContext::getInstance();
  mAppContext->init(mWindow->getGlWindow());
  mCamera = std::make_unique<Camera>(mWindow.get());
}

void Application::run() {
  init();
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

  vkDestroyRenderPass(mAppContext->getDevice(), mGuiPass, nullptr);
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

void Application::createScene() {

  // creates material, loads models from files, creates bvh
  mRtScene = std::make_unique<GpuModel::Scene>();
}

void Application::createBufferBundles() {
  // equals to descriptor sets size
  auto swapchainSize = static_cast<uint32_t>(mAppContext->getSwapchainSize());
  // uniform buffers are faster to fill compared to storage buffers, but they
  // are restricted in their size Buffer bundle is an array of buffers, one per
  // each swapchain image/descriptor set.
  mGlobalBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GlobalUniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  mRtxBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(RtxUniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  mTemperalFilterBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(TemporalFilterUniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  mVarianceBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(VarianceUniformBufferObject),
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
}

void Application::createImagesAndForwardingPairs() {
  auto imageWidth  = mAppContext->getSwapchainExtentWidth();
  auto imageHeight = mAppContext->getSwapchainExtentHeight();

  mTargetImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  // creating forwarding pairs to copy the image result each frame to a specific
  // swapchain
  for (int i = 0; i < mAppContext->getSwapchainSize(); i++) {
    mTargetForwardingPairs.emplace_back(std::make_unique<ImageForwardingPair>(
        mTargetImage->getVkImage(), mAppContext->getSwapchainImages()[i],
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  }

  mRawImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mATrousInputImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mATrousOutputImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
          VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  mATrousForwardingPair = std::make_unique<ImageForwardingPair>(
      mATrousOutputImage->getVkImage(), mATrousInputImage->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

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
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mDepthImagePrev = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mDepthForwardingPair = std::make_unique<ImageForwardingPair>(
      mDepthImage->getVkImage(), mDepthImagePrev->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  mNormalImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mNormalImagePrev = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mNormalForwardingPair = std::make_unique<ImageForwardingPair>(
      mNormalImage->getVkImage(), mNormalImagePrev->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  mGradientImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mGradientImagePrev = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mGradientForwardingPair = std::make_unique<ImageForwardingPair>(
      mGradientImage->getVkImage(), mGradientImagePrev->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  mVarianceHistImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mVarianceHistImagePrev = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mVarianceHistForwardingPair = std::make_unique<ImageForwardingPair>(
      mVarianceHistImage->getVkImage(), mVarianceHistImagePrev->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  mMeshHashImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mMeshHashImagePrev = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  mMeshHashForwardingPair = std::make_unique<ImageForwardingPair>(
      mMeshHashImage->getVkImage(), mMeshHashImagePrev->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  mVarianceImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  mLastFrameAccumImage = std::make_unique<Image>(
      mAppContext->getDevice(), mAppContext->getCommandPool(),
      mAppContext->getGraphicsQueue(), imageWidth, imageHeight,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);
}

// TODO: this step might need to be rebinded after all the resources are
// recreated
void Application::createComputeModels() {
  auto rtxMat = std::make_unique<ComputeMaterial>(kPathToResourceFolder +
                                                  "/shaders/generated/rtx.spv");
  rtxMat->addUniformBufferBundle(mGlobalBufferBundle.get());
  rtxMat->addUniformBufferBundle(mRtxBufferBundle.get());
  // input
  rtxMat->addStorageImage(mPositionImage.get());
  rtxMat->addStorageImage(mNormalImage.get());
  rtxMat->addStorageImage(mDepthImage.get());
  rtxMat->addStorageImage(mMeshHashImage.get());
  // output
  rtxMat->addStorageImage(mRawImage.get());
  // buffers
  rtxMat->addStorageBufferBundle(mTriangleBufferBundle.get());
  rtxMat->addStorageBufferBundle(mMaterialBufferBundle.get());
  rtxMat->addStorageBufferBundle(mBvhBufferBundle.get());
  rtxMat->addStorageBufferBundle(mLightsBufferBundle.get());
  mRtxModel = std::make_unique<ComputeModel>(std::move(rtxMat));

  auto gradientMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/gradient.spv");
  gradientMat->addUniformBufferBundle(mGlobalBufferBundle.get());
  // input
  gradientMat->addStorageImage(mDepthImage.get());
  // output
  gradientMat->addStorageImage(mGradientImage.get());
  mGradientModel = std::make_unique<ComputeModel>(std::move(gradientMat));

  auto temporalFilterMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/temporalFilter.spv");
  temporalFilterMat->addUniformBufferBundle(mGlobalBufferBundle.get());
  temporalFilterMat->addUniformBufferBundle(mTemperalFilterBufferBundle.get());
  // input
  temporalFilterMat->addStorageImage(mPositionImage.get());
  temporalFilterMat->addStorageImage(mRawImage.get());
  temporalFilterMat->addStorageImage(mDepthImage.get());
  temporalFilterMat->addStorageImage(mDepthImagePrev.get());
  temporalFilterMat->addStorageImage(mNormalImage.get());
  temporalFilterMat->addStorageImage(mNormalImagePrev.get());
  temporalFilterMat->addStorageImage(mGradientImage.get());
  temporalFilterMat->addStorageImage(mGradientImagePrev.get());
  temporalFilterMat->addStorageImage(mMeshHashImage.get());
  temporalFilterMat->addStorageImage(mMeshHashImagePrev.get());
  temporalFilterMat->addStorageImage(mLastFrameAccumImage.get());
  temporalFilterMat->addStorageImage(mVarianceHistImagePrev.get());
  // output
  temporalFilterMat->addStorageImage(mATrousInputImage.get());
  temporalFilterMat->addStorageImage(mVarianceHistImage.get());
  mTemporalFilterModel =
      std::make_unique<ComputeModel>(std::move(temporalFilterMat));

  auto varianceMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/variance.spv");
  varianceMat->addUniformBufferBundle(mGlobalBufferBundle.get());
  varianceMat->addUniformBufferBundle(mVarianceBufferBundle.get());
  // input
  varianceMat->addStorageImage(mATrousInputImage.get());
  varianceMat->addStorageImage(mNormalImage.get());
  varianceMat->addStorageImage(mDepthImage.get());
  // output
  varianceMat->addStorageImage(mGradientImage.get());
  varianceMat->addStorageImage(mVarianceImage.get());
  varianceMat->addStorageImage(mVarianceHistImage.get());
  mVarianceModel = std::make_unique<ComputeModel>(std::move(varianceMat));

  for (int i = 0; i < kATrousSize; i++) {
    auto aTrousMat = std::make_unique<ComputeMaterial>(
        kPathToResourceFolder + "/shaders/generated/aTrous.spv");
    aTrousMat->addUniformBufferBundle(mGlobalBufferBundle.get());
    aTrousMat->addUniformBufferBundle(mBlurFilterBufferBundles[i].get());
    // input
    aTrousMat->addStorageImage(mATrousInputImage.get());
    aTrousMat->addStorageImage(mNormalImage.get());
    aTrousMat->addStorageImage(mDepthImage.get());
    aTrousMat->addStorageImage(mGradientImage.get());
    aTrousMat->addStorageImage(mVarianceImage.get());
    // output
    aTrousMat->addStorageImage(mLastFrameAccumImage.get());
    aTrousMat->addStorageImage(mATrousOutputImage.get());
    mATrousModels.emplace_back(
        std::make_unique<ComputeModel>(std::move(aTrousMat)));
  }

  auto postProcessingMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/postProcessing.spv");
  {
    postProcessingMat->addUniformBufferBundle(mGlobalBufferBundle.get());
    // input
    postProcessingMat->addStorageImage(mATrousOutputImage.get());
    // output
    postProcessingMat->addStorageImage(mTargetImage.get());
  }
  mPostProcessingModel =
      std::make_unique<ComputeModel>(std::move(postProcessingMat));
}

void Application::updateScene(uint32_t currentImageIndex) {
  static uint32_t currentSample = 0;
  static glm::mat4 lastMvpe{1.0F};

  auto currentTime = static_cast<float>(glfwGetTime());

  GlobalUniformBufferObject globalUbo = {
      mAppContext->getSwapchainExtentWidth(),
      mAppContext->getSwapchainExtentHeight(), currentSample, currentTime};
  mGlobalBufferBundle->getBuffer(currentImageIndex)->fillData(&globalUbo);

  RtxUniformBufferObject rtxUbo = {
      mCamera->getPosition(),
      mCamera->getFront(),
      mCamera->getUp(),
      mCamera->getRight(),
      mCamera->getVFov(),
      static_cast<uint32_t>(mRtScene->triangles.size()),
      static_cast<uint32_t>(mRtScene->lights.size()),
      mMovingLightSource,
      mUseLdsNoise,
      mOutputType};

  mRtxBufferBundle->getBuffer(currentImageIndex)->fillData(&rtxUbo);

  TemporalFilterUniformBufferObject tfUbo = {!mUseTemporalBlend, mUseNormalTest,
                                             mNormalThreshold, mBlendingAlpha,
                                             lastMvpe};
  {
    mTemperalFilterBufferBundle->getBuffer(currentImageIndex)->fillData(&tfUbo);

    lastMvpe =
        mCamera->getProjectionMatrix(
            static_cast<float>(mAppContext->getSwapchainExtentWidth()) /
            static_cast<float>(mAppContext->getSwapchainExtentHeight())) *
        mCamera->getViewMatrix();
  }

  VarianceUniformBufferObject varianceUbo = {
      !mUseVarianceEstimation, mSkipStoppingFunctions, mUseTemporalVariance,
      mVarianceKernelSize,     mVariancePhiGaussian,   mVariancePhiDepth};
  mVarianceBufferBundle->getBuffer(currentImageIndex)->fillData(&varianceUbo);

  for (int j = 0; j < kATrousSize; j++) {
    // update ubo for the sampleDistance
    BlurFilterUniformBufferObject bfUbo = {!mUseATrous,
                                           j,
                                           mICap,
                                           mShowVariance,
                                           mUseVarianceGuidedFiltering,
                                           mUseGradientInDepth,
                                           mPhiLuminance,
                                           mPhiDepth,
                                           mPhiNormal,
                                           mIgnoreLuminanceAtFirstIteration,
                                           mChangingLuminancePhi,
                                           mUseJittering};
    mBlurFilterBufferBundles[j]->getBuffer(currentImageIndex)->fillData(&bfUbo);
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

  for (size_t i = 0; i < mCommandBuffers.size(); i++) {
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
    vkCmdClearColorImage(currentCommandBuffer, mATrousInputImage->getVkImage(),
                         VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, mATrousOutputImage->getVkImage(),
                         VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);

    mRtxModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                              ceil(mTargetImage->getWidth() / 32.F),
                              ceil(mTargetImage->getHeight() / 32.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 0, nullptr);

    mGradientModel->computeCommand(currentCommandBuffer,
                                   static_cast<uint32_t>(i),
                                   ceil(mTargetImage->getWidth() / 32.F),
                                   ceil(mTargetImage->getHeight() / 32.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 0, nullptr);

    mTemporalFilterModel->computeCommand(
        currentCommandBuffer, static_cast<uint32_t>(i),
        ceil(mTargetImage->getWidth() / 32.F),
        ceil(mTargetImage->getHeight() / 32.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 0, nullptr);

    mVarianceModel->computeCommand(currentCommandBuffer,
                                   static_cast<uint32_t>(i),
                                   ceil(mTargetImage->getWidth() / 32.F),
                                   ceil(mTargetImage->getHeight() / 32.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 0, nullptr);

    /////////////////////////////////////////////

    for (int j = 0; j < kATrousSize; j++) {
      // update ubo for the sampleDistance
      // BlurFilterUniformBufferObject bfUbo = {!mUseATrous, currentSam};
      // {
      //   auto &allocation =
      //       mBlurFilterBufferBundles[j]->getBuffer(i)->getAllocation();
      //   void *data;
      //   vmaMapMemory(mAppContext->getAllocator(), allocation, &data);
      //   memcpy(data, &bfUbo, sizeof(bfUbo));
      //   vmaUnmapMemory(mAppContext->getAllocator(), allocation);
      // }

      // dispatch filter shader
      mATrousModels[j]->computeCommand(
          currentCommandBuffer, static_cast<uint32_t>(i),
          ceil(mTargetImage->getWidth() / 32.F),
          ceil(mTargetImage->getHeight() / 32.F), 1);
      vkCmdPipelineBarrier(currentCommandBuffer,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                           0, nullptr, 0, nullptr);

      // copy aTrousImage2 to aTrousImage1 (excluding the last transfer)
      if (j != kATrousSize - 1) {
        mATrousForwardingPair->forwardCopying(currentCommandBuffer);
      }
    }

    /////////////////////////////////////////////

    mPostProcessingModel->computeCommand(
        currentCommandBuffer, static_cast<uint32_t>(i),
        ceil(mTargetImage->getWidth() / 32.F),
        ceil(mTargetImage->getHeight() / 32.F), 1);

    mTargetForwardingPairs[i]->forwardCopying(currentCommandBuffer);

    // copy to history images
    mDepthForwardingPair->forwardCopying(currentCommandBuffer);
    mNormalForwardingPair->forwardCopying(currentCommandBuffer);
    mGradientForwardingPair->forwardCopying(currentCommandBuffer);
    mVarianceHistForwardingPair->forwardCopying(currentCommandBuffer);
    mMeshHashForwardingPair->forwardCopying(currentCommandBuffer);

    // Bind graphics pipeline and dispatch draw command.
    // postProcessScene->writeRenderCommand(currentCommandBuffer, i);

    result = vkEndCommandBuffer(currentCommandBuffer);
    Logger::checkStep("vkEndCommandBuffer", result);
  }
}

void Application::cleanupImagesAndForwardingPairs() {
  mPositionImage.reset();
  mRawImage.reset();

  mTargetImage.reset();
  mTargetForwardingPairs.clear();

  mLastFrameAccumImage.reset();

  mVarianceImage.reset();

  mDepthImage.reset();
  mDepthImagePrev.reset();
  mDepthForwardingPair.reset();

  mNormalImage.reset();
  mNormalImagePrev.reset();
  mNormalForwardingPair.reset();

  mGradientImage.reset();
  mGradientImagePrev.reset();
  mGradientForwardingPair.reset();

  mVarianceHistImage.reset();
  mVarianceHistImagePrev.reset();
  mVarianceHistForwardingPair.reset();

  mMeshHashImage.reset();
  mMeshHashImagePrev.reset();
  mMeshHashForwardingPair.reset();

  mATrousInputImage.reset();
  mATrousOutputImage.reset();
  mATrousForwardingPair.reset();
}

void Application::cleanupGuiFrameBuffers() {
  for (auto &framebuffer : mGuiFrameBuffers) {
    vkDestroyFramebuffer(mAppContext->getDevice(), framebuffer, nullptr);
  }
}

// these models are binded with image resources
void Application::cleanupComputeModels() {
  mRtxModel.reset();
  mGradientModel.reset();
  mTemporalFilterModel.reset();
  mVarianceModel.reset();
  mATrousModels.clear();
  mPostProcessingModel.reset();
}

// these commandbuffers are initially recorded with the models
void Application::cleanupRenderCommandBuffers() {

  for (auto &commandBuffer : mCommandBuffers) {
    vkFreeCommandBuffers(mAppContext->getDevice(),
                         mAppContext->getCommandPool(), 1, &commandBuffer);
  }
}
void Application::cleanupSwapchainDimensionRelatedResources() {
  cleanupImagesAndForwardingPairs();
  cleanupComputeModels();
  cleanupRenderCommandBuffers();
  cleanupGuiFrameBuffers();
}

void Application::createSwapchainDimensionRelatedResources() {
  createImagesAndForwardingPairs();
  createComputeModels();
  createRenderCommandBuffers();
  createGuiFramebuffers();
}

void Application::vkCreateSemaphoresAndFences() {
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
      mAppContext->getDevice(), &renderPassCreateInfo, nullptr, &mGuiPass);
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
    frameBufferCreateInfo.renderPass      = mGuiPass;
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
      22.0f);

  io.ConfigFlags |= ImGuiWindowFlags_NoNavInputs;

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
  if (!ImGui_ImplVulkan_Init(&init_info, mGuiPass)) {
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
  renderPassInfo.renderPass        = mGuiPass;
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
  if (ImGui::BeginMenu("Config")) {
    ImGui::SeparatorText("Scene");
    ImGui::Checkbox("Moving Light Source", &mMovingLightSource);
    ImGui::Checkbox("Use LDS Noise", &mUseLdsNoise);

    const char *items[] = {"Combined", "Direct Only", "Indirect Only"};
    static const char *currentSelectedItem = items[0];
    if (ImGui::BeginCombo("Output Type",
                          currentSelectedItem)) { // The second parameter is the
                                                  // label previewed
                                                  // before opening the combo.
      for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
        // You can store your selection however you want, outside or inside your
        // objects
        bool is_selected = (currentSelectedItem == items[n]);
        if (ImGui::Selectable(items[n], is_selected)) {
          currentSelectedItem = items[n];
          mOutputType         = n;
        }

        // Set the initial focus when opening the combo (scrolling + for
        // keyboard navigation support in the upcoming navigation branch)
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::SeparatorText("Temporal Blend");
    ImGui::Checkbox("Temporal Accumulation", &mUseTemporalBlend);
    ImGui::Checkbox("Use normal test", &mUseNormalTest);
    ImGui::SliderFloat("Normal threhold", &mNormalThreshold, 0.0F, 1.0F);
    ImGui::SliderFloat("Blending Alpha", &mBlendingAlpha, 0.0F, 1.0F);

    ImGui::SeparatorText("Variance Estimation");
    ImGui::Checkbox("Variance Calculation", &mUseVarianceEstimation);
    ImGui::Checkbox("Skip Stopping Functions", &mSkipStoppingFunctions);
    ImGui::Checkbox("Use Temporal Variance", &mUseTemporalVariance);

    ImGui::SliderInt("Variance Kernel Size", &mVarianceKernelSize, 1, 15);
    ImGui::SliderFloat("Variance Phi Gaussian", &mVariancePhiGaussian, 0.0F,
                       1.0F);
    ImGui::SliderFloat("Variance Phi Depth", &mVariancePhiDepth, 0.0F, 1.0F);

    ImGui::SeparatorText("A-Trous");
    ImGui::Checkbox("A-Trous", &mUseATrous);
    ImGui::Checkbox("Show variance", &mShowVariance);
    ImGui::SliderInt("A-Trous times", &mICap, 0, 5);
    ImGui::Checkbox("Use variance guided filtering",
                    &mUseVarianceGuidedFiltering);
    ImGui::Checkbox("Use gradient in depth", &mUseGradientInDepth);
    ImGui::SliderFloat("Luminance Phi", &mPhiLuminance, 0.0F, 1.0F);
    ImGui::SliderFloat("Phi Depth", &mPhiDepth, 0.0F, 1.0F);
    ImGui::SliderFloat("Phi Normal", &mPhiNormal, 0.0F, 200.0F);
    ImGui::Checkbox("Ignore Luminance For First Iteration",
                    &mIgnoreLuminanceAtFirstIteration);
    ImGui::Checkbox("Changing luminance phi", &mChangingLuminancePhi);
    ImGui::Checkbox("Use jitter", &mUseJittering);
    ImGui::EndMenu();
  }
  ImGui::EndMainMenuBar();

  auto *mainGuiViewPort = ImGui::GetMainViewport();
  // float width           = mainGuiViewPort->Size.x;
  float height = mainGuiViewPort->Size.y;

  const float kStatsWindowWidth  = 200.0F;
  const float kStatsWindowHeight = 80.0F;

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
  ImGui::Text("fps : %.2f", mFps);
  ImGui::Text("mspf: %.2f", 1 / mFps * 1000.F);

  ImGui::End();
  ImGui::Render();
}

void Application::waitForTheWindowToBeResumed() {
  int width  = mWindow->getFrameBufferWidth();
  int height = mWindow->getFrameBufferHeight();

  while (width == 0 || height == 0) {
    glfwWaitEvents();
    width  = mWindow->getFrameBufferWidth();
    height = mWindow->getFrameBufferHeight();
  }
}

void Application::mainLoop() {
  static float fpsFrameCount     = 0;
  static float fpsRecordLastTime = 0;

  while (glfwWindowShouldClose(mWindow->getGlWindow()) == 0) {
    glfwPollEvents();
    auto &io = ImGui::GetIO();
    // the MousePos is problematic when the window is not focused, so we set it
    // manually :<
    io.MousePos = ImVec2(mWindow->getCursorXPos(), mWindow->getCursorYPos());

    if (glfwGetWindowAttrib(mWindow->getGlWindow(), GLFW_FOCUSED) ==
        GLFW_FALSE) {
      Logger::print("window is not focused");
    }

    if (mWindow->windowSizeChanged() || needToToggleWindowStyle()) {
      mWindow->setWindowSizeChanged(false);

      vkDeviceWaitIdle(mAppContext->getDevice());
      waitForTheWindowToBeResumed();

      cleanupSwapchainDimensionRelatedResources();
      mAppContext->cleanupSwapchainDimensionRelatedResources();
      mAppContext->createSwapchainDimensionRelatedResources();
      createSwapchainDimensionRelatedResources();

      fpsFrameCount     = 0;
      fpsRecordLastTime = 0;
      continue;
    }
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

bool Application::needToToggleWindowStyle() {
  if (mWindow->isInputBitActive(GLFW_KEY_F)) {
    mWindow->disableInputBit(GLFW_KEY_F);
    mWindow->toggleWindowStyle();
    return true;
  }
  return false;
}

void Application::init() {
  createScene();
  createBufferBundles();
  createImagesAndForwardingPairs();
  createComputeModels();

  // create render command buffers
  createRenderCommandBuffers();

  // create GUI command buffers
  createGuiCommandBuffers();

  // create synchronization objects
  vkCreateSemaphoresAndFences();

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