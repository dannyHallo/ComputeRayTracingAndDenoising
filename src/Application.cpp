#include "Application.h"

Camera camera{};

void mouseCallback(GLFWwindow *window, double xpos, double ypos) {
  static float lastX, lastY;
  static bool firstMouse = true;

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

void Application::run() {
  initVulkan();
  mainLoop();
  cleanup();
}

void check_vk_result(VkResult resultCode) { logger::checkStep("check_vk_result", resultCode); }

void Application::initScene() {
  // equals to descriptor sets size
  uint32_t swapchainSize = static_cast<uint32_t>(vulkanApplicationContext.getSwapchainSize());

  // creates material, loads models from files, creates bvh
  mRtScene = std::make_shared<GpuModel::Scene>();

  // uniform buffers are faster to fill compared to storage buffers, but they are restricted in their size
  // Buffer bundle is an array of buffers, one per each swapchain image/descriptor set.
  mRtxBufferBundle = std::make_shared<BufferBundle>(swapchainSize, sizeof(RtxUniformBufferObject),
                                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  mTemperalFilterBufferBundle =
      std::make_shared<BufferBundle>(swapchainSize, sizeof(TemporalFilterUniformBufferObject),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < kATrousSize; i++) {
    auto blurFilterBufferBundle =
        std::make_shared<BufferBundle>(swapchainSize, sizeof(BlurFilterUniformBufferObject),
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    mBlurFilterBufferBundles.emplace_back(std::move(blurFilterBufferBundle));
  }

  auto triangleBufferBundle = std::make_shared<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Triangle) * mRtScene->triangles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, mRtScene->triangles.data());

  auto materialBufferBundle = std::make_shared<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Material) * mRtScene->materials.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, mRtScene->materials.data());

  auto bvhBufferBundle = std::make_shared<BufferBundle>(
      swapchainSize, sizeof(GpuModel::BvhNode) * mRtScene->bvhNodes.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, mRtScene->bvhNodes.data());

  auto lightsBufferBundle = std::make_shared<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Light) * mRtScene->lights.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, mRtScene->lights.data());

  mTargetImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                         vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                         VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                         VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mRawImage = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mBlurHImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                        vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                        VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mATrousImage1 = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mATrousImage2 = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  // introducing G-Buffers
  mPositionImage = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mDepthImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                        vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                        VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                        VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mNormalImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                         vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                         VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                         VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mMeshHashImage1 = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mMeshHashImage2 = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  mAccumulationImage = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  // rtx.comp
  auto rtxMat = std::make_shared<ComputeMaterial>(kPathToResourceFolder + "/shaders/generated/rtx.spv");
  {
    rtxMat->addUniformBufferBundle(mRtxBufferBundle);
    // input
    rtxMat->addStorageImage(mPositionImage);
    rtxMat->addStorageImage(mNormalImage);
    rtxMat->addStorageImage(mDepthImage);
    rtxMat->addStorageImage(mMeshHashImage1);
    // output
    rtxMat->addStorageImage(mRawImage);
    // buffers
    rtxMat->addStorageBufferBundle(triangleBufferBundle);
    rtxMat->addStorageBufferBundle(materialBufferBundle);
    rtxMat->addStorageBufferBundle(bvhBufferBundle);
    rtxMat->addStorageBufferBundle(lightsBufferBundle);
  }
  mRtxModel = std::make_shared<ComputeModel>(rtxMat);

  // temporalFilter.comp
  auto temporalFilterMat =
      std::make_shared<ComputeMaterial>(kPathToResourceFolder + "/shaders/generated/temporalFilter.spv");
  {
    temporalFilterMat->addUniformBufferBundle(mTemperalFilterBufferBundle);
    // input
    temporalFilterMat->addStorageImage(mPositionImage);
    temporalFilterMat->addStorageImage(mRawImage);
    temporalFilterMat->addStorageImage(mAccumulationImage);
    temporalFilterMat->addStorageImage(mNormalImage);
    temporalFilterMat->addStorageImage(mDepthImage);
    temporalFilterMat->addStorageImage(mMeshHashImage1);
    temporalFilterMat->addStorageImage(mMeshHashImage2);
    // output
    temporalFilterMat->addStorageImage(mATrousImage1);
  }
  mTemporalFilterModel = std::make_shared<ComputeModel>(temporalFilterMat);

  for (int i = 0; i < kATrousSize; i++) {
    auto blurFilterPhase1Mat =
        std::make_shared<ComputeMaterial>(kPathToResourceFolder + "/shaders/generated/blurPhase1.spv");
    {
      blurFilterPhase1Mat->addUniformBufferBundle(mBlurFilterBufferBundles[i]);
      // input
      blurFilterPhase1Mat->addStorageImage(mATrousImage1);
      blurFilterPhase1Mat->addStorageImage(mNormalImage);
      blurFilterPhase1Mat->addStorageImage(mDepthImage);
      blurFilterPhase1Mat->addStorageImage(mPositionImage);
      // output
      blurFilterPhase1Mat->addStorageImage(mBlurHImage);
    }
    mBlurFilterPhase1Models.emplace_back(std::make_shared<ComputeModel>(blurFilterPhase1Mat));

    auto blurFilterPhase2Mat =
        std::make_shared<ComputeMaterial>(kPathToResourceFolder + "/shaders/generated/blurPhase2.spv");
    {
      blurFilterPhase2Mat->addUniformBufferBundle(mBlurFilterBufferBundles[i]);
      // input
      blurFilterPhase2Mat->addStorageImage(mATrousImage1);
      blurFilterPhase2Mat->addStorageImage(mBlurHImage);
      blurFilterPhase2Mat->addStorageImage(mNormalImage);
      blurFilterPhase2Mat->addStorageImage(mDepthImage);
      blurFilterPhase2Mat->addStorageImage(mPositionImage);
      // output
      blurFilterPhase2Mat->addStorageImage(mATrousImage2);
    }
    mBlurFilterPhase2Models.emplace_back(std::make_shared<ComputeModel>(blurFilterPhase2Mat));
  }

  auto blurFilterPhase3Mat =
      std::make_shared<ComputeMaterial>(kPathToResourceFolder + "/shaders/generated/blurPhase3.spv");
  {
    // input
    blurFilterPhase3Mat->addStorageImage(mATrousImage2);
    blurFilterPhase3Mat->addStorageImage(mNormalImage);
    blurFilterPhase3Mat->addStorageImage(mDepthImage);
    blurFilterPhase3Mat->addStorageImage(mPositionImage);
    // output
    blurFilterPhase3Mat->addStorageImage(mTargetImage);
  }
  mBlurFilterPhase3Model = std::make_shared<ComputeModel>(blurFilterPhase3Mat);
}

void Application::updateScene(uint32_t currentImage) {
  static uint32_t currentSample = 0;
  static glm::mat4 lastMvpe{1.0f};

  float currentTime = (float)glfwGetTime();

  RtxUniformBufferObject rtxUbo = {camera.position,
                                   camera.front,
                                   camera.up,
                                   camera.right,
                                   camera.vFov,
                                   currentTime,
                                   currentSample,
                                   (uint32_t)mRtScene->triangles.size(),
                                   (uint32_t)mRtScene->lights.size()};

  mRtxBufferBundle->getBuffer(currentImage)->fillData(&rtxUbo);

  TemporalFilterUniformBufferObject tfUbo = {!mUseTemporal, lastMvpe,
                                             vulkanApplicationContext.getSwapchainExtentWidth(),
                                             vulkanApplicationContext.getSwapchainExtentHeight()};
  {
    mTemperalFilterBufferBundle->getBuffer(currentImage)->fillData(&tfUbo);

    lastMvpe = camera.getProjectionMatrix(static_cast<float>(vulkanApplicationContext.getSwapchainExtentWidth()) /
                                          static_cast<float>(vulkanApplicationContext.getSwapchainExtentHeight())) *
               camera.getViewMatrix();
  }

  for (int j = 0; j < kATrousSize; j++) {
    // update ubo for the sampleDistance
    BlurFilterUniformBufferObject bfUbo = {!mUseBlur, j};
    mBlurFilterBufferBundles[j]->getBuffer(currentImage)->fillData(&bfUbo);
  }

  currentSample++;
}

void Application::createRenderCommandBuffers() {
  // create command buffers per swapchain image
  mCommandBuffers.resize(vulkanApplicationContext.getSwapchainSize());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = vulkanApplicationContext.getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)mCommandBuffers.size();

  VkResult result = vkAllocateCommandBuffers(vulkanApplicationContext.getDevice(), &allocInfo, mCommandBuffers.data());
  logger::checkStep("vkAllocateCommandBuffers", result);

  VkImageMemoryBarrier targetTexGeneral2TransSrc = ImageUtils::generalToTransferSrcBarrier(mTargetImage->getVkImage());
  VkImageMemoryBarrier targetTexTransSrc2General = ImageUtils::transferSrcToGeneralBarrier(mTargetImage->getVkImage());

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

  VkImageCopy imgCopyRegion = ImageUtils::imageCopyRegion(vulkanApplicationContext.getSwapchainExtentWidth(),
                                                          vulkanApplicationContext.getSwapchainExtentHeight());

  for (size_t i = 0; i < mCommandBuffers.size(); i++) {
    VkImageMemoryBarrier swapchainImageUndefined2TransferDst =
        ImageUtils::undefinedToTransferDstBarrier(vulkanApplicationContext.getSwapchainImages()[i]);
    VkImageMemoryBarrier swapchainImageTransferDst2ColorAttachment =
        ImageUtils::transferDstToColorAttachmentBarrier(vulkanApplicationContext.getSwapchainImages()[i]);

    VkCommandBuffer &currentCommandBuffer = mCommandBuffers[i];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(currentCommandBuffer, &beginInfo);
    logger::checkStep("vkBeginCommandBuffer", result);

    VkClearColorValue clearColor{0, 0, 0, 0};
    VkImageSubresourceRange clearRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(currentCommandBuffer, mTargetImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1,
                         &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, mATrousImage1->getVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1,
                         &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, mATrousImage2->getVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1,
                         &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, mBlurHImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1,
                         &clearRange);

    mRtxModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), mTargetImage->getWidth() / 32,
                              mTargetImage->getHeight() / 32, 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

    mTemporalFilterModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), mTargetImage->getWidth() / 32,
                                         mTargetImage->getHeight() / 32, 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

    /////////////////////////////////////////////

    for (int j = 0; j < kATrousSize; j++) {
      // update ubo for the sampleDistance
      BlurFilterUniformBufferObject bfUbo = {!mUseBlur, j};
      {
        auto &allocation = mBlurFilterBufferBundles[j]->getBuffer(i)->getAllocation();
        void *data;
        vmaMapMemory(vulkanApplicationContext.getAllocator(), allocation, &data);
        memcpy(data, &bfUbo, sizeof(bfUbo));
        vmaUnmapMemory(vulkanApplicationContext.getAllocator(), allocation);
      }

      // dispatch 2 stages of separate shaders
      mBlurFilterPhase1Models[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                                 mTargetImage->getWidth() / 32, mTargetImage->getHeight() / 32, 1);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
      mBlurFilterPhase2Models[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                                 mTargetImage->getWidth() / 32, mTargetImage->getHeight() / 32, 1);

      // accum texture when the first filter is done
      // (from SVGF) accumulation image should use a properly smoothed image,
      // the presented one is not ideal due to the over-blur over time
      if (j == 0) {
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex2General2TransSrc);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &accumTexGeneral2TransDst);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex1General2TransDst);
        vkCmdCopyImage(currentCommandBuffer, mATrousImage2->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       mATrousImage1->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdCopyImage(currentCommandBuffer, mATrousImage2->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       mAccumulationImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex1TransDst2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex2TransSrc2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &accumTexTransDst2General);
      }
      // copy aTrousImage2 to aTrousImage1 (excluding the last transfer)
      else if (j != kATrousSize - 1) {
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex1General2TransDst);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex2General2TransSrc);
        vkCmdCopyImage(currentCommandBuffer, mATrousImage2->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       mATrousImage1->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex1TransDst2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex2TransSrc2General);
      }
    }

    /////////////////////////////////////////////

    mBlurFilterPhase3Model->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                           mTargetImage->getWidth() / 32, mTargetImage->getHeight() / 32, 1);

    // copy targetTex to swapchainImage
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &targetTexGeneral2TransSrc);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &swapchainImageUndefined2TransferDst);
    vkCmdCopyImage(currentCommandBuffer, mTargetImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   vulkanApplicationContext.getSwapchainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                   &imgCopyRegion);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &targetTexTransSrc2General);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &swapchainImageTransferDst2ColorAttachment);

    // sync mesh hash image for next frame
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &triIdTex1General2TransSrc);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &triIdTex2General2TransDst);
    vkCmdCopyImage(currentCommandBuffer, mMeshHashImage1->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   mMeshHashImage2->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &triIdTex1TransSrc2General);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &triIdTex2TransDst2General);

    // Bind graphics pipeline and dispatch draw command.
    // postProcessScene->writeRenderCommand(currentCommandBuffer, i);

    result = vkEndCommandBuffer(currentCommandBuffer);
    logger::checkStep("vkEndCommandBuffer", result);
  }
}

void Application::createSyncObjects() {
  mImageAvailableSemaphores.resize(kMaxFramesInFlight);
  mRenderFinishedSemaphores.resize(kMaxFramesInFlight);
  mFramesInFlightFences.resize(kMaxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  // the first call to vkWaitForFences() returns immediately since the fence is already signaled
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < kMaxFramesInFlight; i++) {
    if (vkCreateSemaphore(vulkanApplicationContext.getDevice(), &semaphoreInfo, nullptr,
                          &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(vulkanApplicationContext.getDevice(), &semaphoreInfo, nullptr,
                          &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(vulkanApplicationContext.getDevice(), &fenceInfo, nullptr, &mFramesInFlightFences[i]) !=
            VK_SUCCESS) {
      logger::throwError("failed to create synchronization objects for a frame!");
    }
  }
}

void Application::createGuiCommandBuffers() {
  mGuiCommandBuffers.resize(vulkanApplicationContext.getSwapchainSize());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = vulkanApplicationContext.getGuiCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)mGuiCommandBuffers.size();

  VkResult result =
      vkAllocateCommandBuffers(vulkanApplicationContext.getDevice(), &allocInfo, mGuiCommandBuffers.data());
  logger::checkStep("vkAllocateCommandBuffers", result);
}

void Application::createGuiRenderPass() {
  // Imgui Pass, right after the main pass
  VkAttachmentDescription attachment = {};
  attachment.format                  = vulkanApplicationContext.getSwapchainImageFormat();
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
  colorAttachment.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachment;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount        = 1;
  renderPassCreateInfo.pAttachments           = &attachment;
  renderPassCreateInfo.subpassCount           = 1;
  renderPassCreateInfo.pSubpasses             = &subpass;
  renderPassCreateInfo.dependencyCount        = 1;
  renderPassCreateInfo.pDependencies          = &dependency;

  VkResult result =
      vkCreateRenderPass(vulkanApplicationContext.getDevice(), &renderPassCreateInfo, nullptr, &mImGuiPass);
  logger::checkStep("vkCreateRenderPass", result);
}

void Application::createGuiFramebuffers() {
  // Create gui frame buffers for gui pass to use
  // Each frame buffer will have an attachment of VkImageView, in this case, the attachments are
  // mSwapchainImageViews
  mSwapchainGuiFrameBuffers.resize(vulkanApplicationContext.getSwapchainSize());

  // Iterate through image views
  for (size_t i = 0; i < vulkanApplicationContext.getSwapchainSize(); i++) {
    VkImageView attachment = vulkanApplicationContext.getSwapchainImageViews()[i];

    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass      = mImGuiPass;
    frameBufferCreateInfo.attachmentCount = 1;
    frameBufferCreateInfo.pAttachments    = &attachment;
    frameBufferCreateInfo.width           = vulkanApplicationContext.getSwapchainExtentWidth();
    frameBufferCreateInfo.height          = vulkanApplicationContext.getSwapchainExtentHeight();
    frameBufferCreateInfo.layers          = 1;

    VkResult result = vkCreateFramebuffer(vulkanApplicationContext.getDevice(), &frameBufferCreateInfo, nullptr,
                                          &mSwapchainGuiFrameBuffers[i]);
    logger::checkStep("vkCreateFramebuffer", result);
  }
}

void Application::createGuiDescripterPool() {
  VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
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
  poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets                    = 1000 * IM_ARRAYSIZE(poolSizes);
  poolInfo.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(poolSizes);
  poolInfo.pPoolSizes                 = poolSizes;

  VkResult result =
      vkCreateDescriptorPool(vulkanApplicationContext.getDevice(), &poolInfo, nullptr, &mGuiDescriptorPool);
  logger::checkStep("vkCreateDescriptorPool", result);
}

VkCommandBuffer Application::beginSingleTimeCommands() {
  VkCommandBuffer commandBuffer;

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool        = vulkanApplicationContext.getCommandPool();
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(vulkanApplicationContext.getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
    logger::print("failed to allocate command buffers!");

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Application::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit(vulkanApplicationContext.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(vulkanApplicationContext.getGraphicsQueue());

  vkFreeCommandBuffers(vulkanApplicationContext.getDevice(), vulkanApplicationContext.getCommandPool(), 1,
                       &commandBuffer);
}

void Application::initGui() {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  // Does nothing, avoid compiler warning
  (void)io;

  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  // Setup Dear ImGui style
  // ImGui::StyleColorsDark();
  ImGui::StyleColorsClassic();

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForVulkan(vulkanApplicationContext.getWindow(), true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = vulkanApplicationContext.getInstance();
  init_info.PhysicalDevice            = vulkanApplicationContext.getPhysicalDevice();
  init_info.Device                    = vulkanApplicationContext.getDevice();
  init_info.QueueFamily               = vulkanApplicationContext.getGraphicsFamilyIndex();
  init_info.Queue                     = vulkanApplicationContext.getGraphicsQueue();
  init_info.PipelineCache             = VK_NULL_HANDLE;
  init_info.DescriptorPool            = mGuiDescriptorPool;
  init_info.Allocator                 = VK_NULL_HANDLE;
  init_info.MinImageCount             = static_cast<uint32_t>(vulkanApplicationContext.getSwapchainSize());
  init_info.ImageCount                = static_cast<uint32_t>(vulkanApplicationContext.getSwapchainSize());
  init_info.CheckVkResultFn           = check_vk_result;
  if (!ImGui_ImplVulkan_Init(&init_info, mImGuiPass)) {
    logger::print("failed to init impl");
  }

  // Create fonts texture
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();
  if (!ImGui_ImplVulkan_CreateFontsTexture(commandBuffer)) logger::print("failed to create fonts texture");
  endSingleTimeCommands(commandBuffer);
}

void Application::recordGuiCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags            = 0;       // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  // A call to vkBeginCommandBuffer will implicitly reset the command buffer
  VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  logger::checkStep("vkBeginCommandBuffer", result);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass            = mImGuiPass;
  renderPassInfo.framebuffer           = mSwapchainGuiFrameBuffers[imageIndex];
  renderPassInfo.renderArea.extent     = vulkanApplicationContext.getSwapchainExtent();

  VkClearValue clearValue{};
  clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues    = &clearValue;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Record Imgui Draw Data and draw funcs into command buffer
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  result = vkEndCommandBuffer(commandBuffer);
  logger::checkStep("vkEndCommandBuffer", result);
}

void Application::drawFrame() {
  static size_t currentFrame = 0;
  vkWaitForFences(vulkanApplicationContext.getDevice(), 1, &mFramesInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(vulkanApplicationContext.getDevice(), 1, &mFramesInFlightFences[currentFrame]);

  uint32_t imageIndex;
  VkResult result =
      vkAcquireNextImageKHR(vulkanApplicationContext.getDevice(), vulkanApplicationContext.getSwapchain(), UINT64_MAX,
                            mImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    // sub-optimal: a swapchain no longer matches the surface properties exactly, but can still be used to present
    // to the surface successfully
    logger::throwError("resizing is not allowed!");
  }

  updateScene(imageIndex);

  recordGuiCommandBuffer(mGuiCommandBuffers[imageIndex], imageIndex);
  std::array<VkCommandBuffer, 2> submitCommandBuffers = {mCommandBuffers[imageIndex], mGuiCommandBuffers[imageIndex]};

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
    submitInfo.pWaitDstStageMask      = waitStages;

    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers    = submitCommandBuffers.data();
  }

  result =
      vkQueueSubmit(vulkanApplicationContext.getGraphicsQueue(), 1, &submitInfo, mFramesInFlightFences[currentFrame]);
  logger::checkStep("vkQueueSubmit", result);

  VkPresentInfoKHR presentInfo{};
  {
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &mRenderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &vulkanApplicationContext.getSwapchain();
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;
  }

  result = vkQueuePresentKHR(vulkanApplicationContext.getPresentQueue(), &presentInfo);
  logger::checkStep("vkQueuePresentKHR", result);

  // Commented this out for playing around with it later :)
  // vkQueueWaitIdle(context.getPresentQueue());
  currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
}

void Application::prepareGui() {
  ImGui_ImplVulkan_NewFrame();

  // handle the user inputs, the screen resize
  ImGui_ImplGlfw_NewFrame();

  ImGui::NewFrame();

  // ImGui::ShowDemoWindow();
  // ImGui::ShowStackToolWindow();

  ImGui::Begin("little gui ( press TAB to use me )");
  ImGui::Text((std::to_string(static_cast<int>(mFps)) + " frames per second").c_str());
  ImGui::Text((std::to_string(static_cast<int>(mFrameTime * 1000)) + " ms per frame").c_str());

  ImGui::Separator();

  // ImGui::Checkbox("Temporal Accumulation", &mUseTemporal);
  // ImGui::Checkbox("A-Trous", &mUseBlur);`

  ImGui::BeginMainMenuBar();
  if (ImGui::BeginMenu("Config")) {
    ImGui::Checkbox("Temporal Accumulation", &mUseTemporal);
    ImGui::Checkbox("A-Trous", &mUseBlur);
    ImGui::EndMenu();
  }
  ImGui::EndMainMenuBar();

  ImGui::End();
  ImGui::Render();
}

void Application::mainLoop() {
  static float fpsFrameCount = 0, fpsRecordLastTime = 0;

  while (!glfwWindowShouldClose(vulkanApplicationContext.getWindow())) {
    glfwPollEvents();

    prepareGui();

    fpsFrameCount++;
    float currentTime    = static_cast<float>(glfwGetTime());
    mDeltaTime           = currentTime - mFrameRecordLastTime;
    mFrameRecordLastTime = currentTime;

    if (currentTime - fpsRecordLastTime >= kFpsUpdateTime) {
      mFps              = fpsFrameCount / kFpsUpdateTime;
      mFrameTime        = 1 / mFps;
      std::string title = "FPS - " + std::to_string(static_cast<int>(mFps));
      glfwSetWindowTitle(vulkanApplicationContext.getWindow(), title.c_str());
      fpsFrameCount = 0;

      fpsRecordLastTime = currentTime;
    }

    camera.processInput(mDeltaTime);
    drawFrame();
  }

  vkDeviceWaitIdle(vulkanApplicationContext.getDevice());
}

void Application::initVulkan() {
  // initialize camera object with the vulkanApplicationContext's window class
  camera.init(&vulkanApplicationContext.getWindowClass());

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

  // set mouse callback function to be called whenever the cursor position changes
  glfwSetCursorPosCallback(vulkanApplicationContext.getWindow(), mouseCallback);
}

void Application::cleanup() {
  for (size_t i = 0; i < kMaxFramesInFlight; i++) {
    vkDestroySemaphore(vulkanApplicationContext.getDevice(), mRenderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(vulkanApplicationContext.getDevice(), mImageAvailableSemaphores[i], nullptr);
    vkDestroyFence(vulkanApplicationContext.getDevice(), mFramesInFlightFences[i], nullptr);
  }

  glfwTerminate();
}