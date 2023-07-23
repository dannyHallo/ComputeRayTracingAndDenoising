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
  rtScene = std::make_shared<GpuModel::Scene>();

  // uniform buffers are faster to fill compared to storage buffers, but they are restricted in their size
  // Buffer bundle is an array of buffers, one per each swapchain image/descriptor set.
  rtxBufferBundle = std::make_shared<BufferBundle>(swapchainSize, sizeof(RtxUniformBufferObject),
                                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  temperalFilterBufferBundle =
      std::make_shared<BufferBundle>(swapchainSize, sizeof(TemporalFilterUniformBufferObject),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < cInFrameProcessSize; i++) {
    auto blurFilterBufferBundle =
        std::make_shared<BufferBundle>(swapchainSize, sizeof(BlurFilterUniformBufferObject),
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    blurFilterBufferBundles.emplace_back(std::move(blurFilterBufferBundle));
  }

  auto triangleBufferBundle = std::make_shared<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Triangle) * rtScene->triangles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->triangles.data());

  auto materialBufferBundle = std::make_shared<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Material) * rtScene->materials.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->materials.data());

  auto bvhBufferBundle = std::make_shared<BufferBundle>(
      swapchainSize, sizeof(GpuModel::BvhNode) * rtScene->bvhNodes.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->bvhNodes.data());

  auto lightsBufferBundle = std::make_shared<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Light) * rtScene->lights.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->lights.data());

  targetImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                        vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                                            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                        VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  rawImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                     vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                     VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT,
                                     VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  blurHImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                       vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                       VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                       VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                       VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  aTrousImage1 = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                         vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                         VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                         VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  aTrousImage2 = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  // introducing G-Buffers
  positionImage = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  depthImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                       vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                       VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                       VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                       VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  normalImage = std::make_shared<Image>(vulkanApplicationContext.getSwapchainExtentWidth(),
                                        vulkanApplicationContext.getSwapchainExtentHeight(), 1, VK_SAMPLE_COUNT_1_BIT,
                                        VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                        VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  meshHashImage1 = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  meshHashImage2 = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  accumulationImage = std::make_shared<Image>(
      vulkanApplicationContext.getSwapchainExtentWidth(), vulkanApplicationContext.getSwapchainExtentHeight(), 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_LAYOUT_GENERAL);

  // rtx.comp
  auto rtxMat = std::make_shared<ComputeMaterial>(pathToResourceFolder + "/shaders/generated/rtx.spv");
  {
    rtxMat->addUniformBufferBundle(rtxBufferBundle);
    // input
    rtxMat->addStorageImage(positionImage);
    rtxMat->addStorageImage(normalImage);
    rtxMat->addStorageImage(depthImage);
    rtxMat->addStorageImage(meshHashImage1);
    // output
    rtxMat->addStorageImage(rawImage);
    // buffers
    rtxMat->addStorageBufferBundle(triangleBufferBundle);
    rtxMat->addStorageBufferBundle(materialBufferBundle);
    rtxMat->addStorageBufferBundle(bvhBufferBundle);
    rtxMat->addStorageBufferBundle(lightsBufferBundle);
  }
  rtxModel = std::make_shared<ComputeModel>(rtxMat);

  // temporalFilter.comp
  auto temporalFilterMat =
      std::make_shared<ComputeMaterial>(pathToResourceFolder + "/shaders/generated/temporalFilter.spv");
  {
    temporalFilterMat->addUniformBufferBundle(temperalFilterBufferBundle);
    // input
    temporalFilterMat->addStorageImage(positionImage);
    temporalFilterMat->addStorageImage(rawImage);
    temporalFilterMat->addStorageImage(accumulationImage);
    temporalFilterMat->addStorageImage(normalImage);
    temporalFilterMat->addStorageImage(depthImage);
    temporalFilterMat->addStorageImage(meshHashImage1);
    temporalFilterMat->addStorageImage(meshHashImage2);
    // output
    temporalFilterMat->addStorageImage(aTrousImage1);
  }
  temporalFilterModel = std::make_shared<ComputeModel>(temporalFilterMat);

  for (int i = 0; i < cInFrameProcessSize; i++) {
    auto blurFilterPhase1Mat =
        std::make_shared<ComputeMaterial>(pathToResourceFolder + "/shaders/generated/blurPhase1.spv");
    {
      blurFilterPhase1Mat->addUniformBufferBundle(blurFilterBufferBundles[i]);
      // input
      blurFilterPhase1Mat->addStorageImage(aTrousImage1);
      blurFilterPhase1Mat->addStorageImage(normalImage);
      blurFilterPhase1Mat->addStorageImage(depthImage);
      blurFilterPhase1Mat->addStorageImage(positionImage);
      // output
      blurFilterPhase1Mat->addStorageImage(blurHImage);
    }
    blurFilterPhase1Models.emplace_back(std::make_shared<ComputeModel>(blurFilterPhase1Mat));

    auto blurFilterPhase2Mat =
        std::make_shared<ComputeMaterial>(pathToResourceFolder + "/shaders/generated/blurPhase2.spv");
    {
      blurFilterPhase2Mat->addUniformBufferBundle(blurFilterBufferBundles[i]);
      // input
      blurFilterPhase2Mat->addStorageImage(aTrousImage1);
      blurFilterPhase2Mat->addStorageImage(blurHImage);
      blurFilterPhase2Mat->addStorageImage(normalImage);
      blurFilterPhase2Mat->addStorageImage(depthImage);
      blurFilterPhase2Mat->addStorageImage(positionImage);
      // output
      blurFilterPhase2Mat->addStorageImage(aTrousImage2);
    }
    blurFilterPhase2Models.emplace_back(std::make_shared<ComputeModel>(blurFilterPhase2Mat));
  }

  auto blurFilterPhase3Mat =
      std::make_shared<ComputeMaterial>(pathToResourceFolder + "/shaders/generated/blurPhase3.spv");
  {
    // input
    blurFilterPhase3Mat->addStorageImage(aTrousImage2);
    blurFilterPhase3Mat->addStorageImage(normalImage);
    blurFilterPhase3Mat->addStorageImage(depthImage);
    blurFilterPhase3Mat->addStorageImage(positionImage);
    // output
    blurFilterPhase3Mat->addStorageImage(targetImage);
  }
  blurFilterPhase3Model = std::make_shared<ComputeModel>(blurFilterPhase3Mat);
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
                                   (uint32_t)rtScene->triangles.size(),
                                   (uint32_t)rtScene->lights.size()};

  rtxBufferBundle->getBuffer(currentImage)->fillData(&rtxUbo);

  TemporalFilterUniformBufferObject tfUbo = {!useTemporal, lastMvpe, vulkanApplicationContext.getSwapchainExtentWidth(),
                                             vulkanApplicationContext.getSwapchainExtentHeight()};
  {
    temperalFilterBufferBundle->getBuffer(currentImage)->fillData(&tfUbo);

    lastMvpe = camera.getProjectionMatrix(static_cast<float>(vulkanApplicationContext.getSwapchainExtentWidth()) /
                                          static_cast<float>(vulkanApplicationContext.getSwapchainExtentHeight())) *
               camera.getViewMatrix();
  }

  for (int j = 0; j < cInFrameProcessSize; j++) {
    // update ubo for the sampleDistance
    BlurFilterUniformBufferObject bfUbo = {!useBlur, j};
    blurFilterBufferBundles[j]->getBuffer(currentImage)->fillData(&bfUbo);
  }

  currentSample++;
}

void Application::createRenderCommandBuffers() {
  // create command buffers per swapchain image
  commandBuffers.resize(vulkanApplicationContext.getSwapchainSize());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = vulkanApplicationContext.getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  VkResult result = vkAllocateCommandBuffers(vulkanApplicationContext.getDevice(), &allocInfo, commandBuffers.data());
  logger::checkStep("vkAllocateCommandBuffers", result);

  VkImageMemoryBarrier targetTexGeneral2TransSrc = ImageUtils::generalToTransferSrcBarrier(targetImage->getVkImage());
  VkImageMemoryBarrier targetTexTransSrc2General = ImageUtils::transferSrcToGeneralBarrier(targetImage->getVkImage());

  VkImageMemoryBarrier accumTexGeneral2TransDst =
      ImageUtils::generalToTransferDstBarrier(accumulationImage->getVkImage());
  VkImageMemoryBarrier accumTexTransDst2General =
      ImageUtils::transferDstToGeneralBarrier(accumulationImage->getVkImage());

  VkImageMemoryBarrier aTrousTex1General2TransDst = ImageUtils::generalToTransferDstBarrier(aTrousImage1->getVkImage());
  VkImageMemoryBarrier aTrousTex1TransDst2General = ImageUtils::transferDstToGeneralBarrier(aTrousImage1->getVkImage());
  VkImageMemoryBarrier aTrousTex2General2TransSrc = ImageUtils::generalToTransferSrcBarrier(aTrousImage2->getVkImage());
  VkImageMemoryBarrier aTrousTex2TransSrc2General = ImageUtils::transferSrcToGeneralBarrier(aTrousImage2->getVkImage());

  VkImageMemoryBarrier triIdTex1General2TransSrc =
      ImageUtils::generalToTransferSrcBarrier(meshHashImage1->getVkImage());
  VkImageMemoryBarrier triIdTex1TransSrc2General =
      ImageUtils::transferSrcToGeneralBarrier(meshHashImage1->getVkImage());
  VkImageMemoryBarrier triIdTex2General2TransDst =
      ImageUtils::generalToTransferDstBarrier(meshHashImage2->getVkImage());
  VkImageMemoryBarrier triIdTex2TransDst2General =
      ImageUtils::transferDstToGeneralBarrier(meshHashImage2->getVkImage());

  VkImageCopy imgCopyRegion = ImageUtils::imageCopyRegion(vulkanApplicationContext.getSwapchainExtentWidth(),
                                                          vulkanApplicationContext.getSwapchainExtentHeight());

  for (size_t i = 0; i < commandBuffers.size(); i++) {
    VkImageMemoryBarrier swapchainImageUndefined2TransferDst =
        ImageUtils::undefinedToTransferDstBarrier(vulkanApplicationContext.getSwapchainImages()[i]);
    VkImageMemoryBarrier swapchainImageTransferDst2ColorAttachment =
        ImageUtils::transferDstToColorAttachmentBarrier(vulkanApplicationContext.getSwapchainImages()[i]);

    VkCommandBuffer &currentCommandBuffer = commandBuffers[i];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(currentCommandBuffer, &beginInfo);
    logger::checkStep("vkBeginCommandBuffer", result);

    VkClearColorValue clearColor{0, 0, 0, 0};
    VkImageSubresourceRange clearRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(currentCommandBuffer, targetImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1,
                         &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, aTrousImage1->getVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1,
                         &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, aTrousImage2->getVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1,
                         &clearRange);
    vkCmdClearColorImage(currentCommandBuffer, blurHImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1,
                         &clearRange);

    rtxModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->getWidth() / 32,
                             targetImage->getHeight() / 32, 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

    temporalFilterModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->getWidth() / 32,
                                        targetImage->getHeight() / 32, 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

    /////////////////////////////////////////////

    for (int j = 0; j < cInFrameProcessSize; j++) {
      // update ubo for the sampleDistance
      BlurFilterUniformBufferObject bfUbo = {!useBlur, j};
      {
        auto &allocation = blurFilterBufferBundles[j]->getBuffer(i)->getAllocation();
        void *data;
        vmaMapMemory(vulkanApplicationContext.getAllocator(), allocation, &data);
        memcpy(data, &bfUbo, sizeof(bfUbo));
        vmaUnmapMemory(vulkanApplicationContext.getAllocator(), allocation);
      }

      // dispatch 2 stages of separate shaders
      blurFilterPhase1Models[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                                targetImage->getWidth() / 32, targetImage->getHeight() / 32, 1);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
      blurFilterPhase2Models[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                                targetImage->getWidth() / 32, targetImage->getHeight() / 32, 1);

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
        vkCmdCopyImage(currentCommandBuffer, aTrousImage2->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       aTrousImage1->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdCopyImage(currentCommandBuffer, aTrousImage2->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       accumulationImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex1TransDst2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex2TransSrc2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &accumTexTransDst2General);
      }
      // copy aTrousImage2 to aTrousImage1 (excluding the last transfer)
      else if (j != cInFrameProcessSize - 1) {
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex1General2TransDst);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex2General2TransSrc);
        vkCmdCopyImage(currentCommandBuffer, aTrousImage2->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       aTrousImage1->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex1TransDst2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &aTrousTex2TransSrc2General);
      }
    }

    /////////////////////////////////////////////

    blurFilterPhase3Model->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->getWidth() / 32,
                                          targetImage->getHeight() / 32, 1);

    // copy targetTex to swapchainImage
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &targetTexGeneral2TransSrc);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &swapchainImageUndefined2TransferDst);
    vkCmdCopyImage(currentCommandBuffer, targetImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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
    vkCmdCopyImage(currentCommandBuffer, meshHashImage1->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   meshHashImage2->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
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
  imageAvailableSemaphores.resize(maxFramesInFlight);
  renderFinishedSemaphores.resize(maxFramesInFlight);
  framesInFlightFences.resize(maxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  // the first call to vkWaitForFences() returns immediately since the fence is already signaled
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < maxFramesInFlight; i++) {
    if (vkCreateSemaphore(vulkanApplicationContext.getDevice(), &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(vulkanApplicationContext.getDevice(), &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(vulkanApplicationContext.getDevice(), &fenceInfo, nullptr, &framesInFlightFences[i]) !=
            VK_SUCCESS) {
      logger::throwError("failed to create synchronization objects for a frame!");
    }
  }
}

void Application::createGuiCommandBuffers() {
  guiCommandBuffers.resize(vulkanApplicationContext.getSwapchainSize());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = vulkanApplicationContext.getGuiCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)guiCommandBuffers.size();

  VkResult result =
      vkAllocateCommandBuffers(vulkanApplicationContext.getDevice(), &allocInfo, guiCommandBuffers.data());
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
      vkCreateRenderPass(vulkanApplicationContext.getDevice(), &renderPassCreateInfo, nullptr, &imGuiPass);
  logger::checkStep("vkCreateRenderPass", result);
}

void Application::createGuiFramebuffers() {
  // Create gui frame buffers for gui pass to use
  // Each frame buffer will have an attachment of VkImageView, in this case, the attachments are
  // mSwapchainImageViews
  swapchainGuiFrameBuffers.resize(vulkanApplicationContext.getSwapchainSize());

  // Iterate through image views
  for (size_t i = 0; i < vulkanApplicationContext.getSwapchainSize(); i++) {
    VkImageView attachment = vulkanApplicationContext.getSwapchainImageViews()[i];

    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass      = imGuiPass;
    frameBufferCreateInfo.attachmentCount = 1;
    frameBufferCreateInfo.pAttachments    = &attachment;
    frameBufferCreateInfo.width           = vulkanApplicationContext.getSwapchainExtentWidth();
    frameBufferCreateInfo.height          = vulkanApplicationContext.getSwapchainExtentHeight();
    frameBufferCreateInfo.layers          = 1;

    VkResult result = vkCreateFramebuffer(vulkanApplicationContext.getDevice(), &frameBufferCreateInfo, nullptr,
                                          &swapchainGuiFrameBuffers[i]);
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
      vkCreateDescriptorPool(vulkanApplicationContext.getDevice(), &poolInfo, nullptr, &guiDescriptorPool);
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
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForVulkan(vulkanApplicationContext.getWindow(), true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = vulkanApplicationContext.getInstance();
  init_info.PhysicalDevice            = vulkanApplicationContext.getPhysicalDevice();
  init_info.Device                    = vulkanApplicationContext.getDevice();
  init_info.QueueFamily               = vulkanApplicationContext.getGraphicsFamilyIndex();
  init_info.Queue                     = vulkanApplicationContext.getGraphicsQueue();
  init_info.PipelineCache             = VK_NULL_HANDLE;
  init_info.DescriptorPool            = guiDescriptorPool;
  init_info.Allocator                 = VK_NULL_HANDLE;
  init_info.MinImageCount             = static_cast<uint32_t>(vulkanApplicationContext.getSwapchainSize());
  init_info.ImageCount                = static_cast<uint32_t>(vulkanApplicationContext.getSwapchainSize());
  init_info.CheckVkResultFn           = check_vk_result;
  if (!ImGui_ImplVulkan_Init(&init_info, imGuiPass)) {
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
  renderPassInfo.renderPass            = imGuiPass;
  renderPassInfo.framebuffer           = swapchainGuiFrameBuffers[imageIndex];
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
  vkWaitForFences(vulkanApplicationContext.getDevice(), 1, &framesInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(vulkanApplicationContext.getDevice(), 1, &framesInFlightFences[currentFrame]);

  uint32_t imageIndex;
  VkResult result =
      vkAcquireNextImageKHR(vulkanApplicationContext.getDevice(), vulkanApplicationContext.getSwapchain(), UINT64_MAX,
                            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    // sub-optimal: a swapchain no longer matches the surface properties exactly, but can still be used to present
    // to the surface successfully
    logger::throwError("resizing is not allowed!");
  }

  updateScene(imageIndex);

  recordGuiCommandBuffer(guiCommandBuffers[imageIndex], imageIndex);
  std::array<VkCommandBuffer, 2> submitCommandBuffers = {commandBuffers[imageIndex], guiCommandBuffers[imageIndex]};

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

    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers    = submitCommandBuffers.data();
  }

  result =
      vkQueueSubmit(vulkanApplicationContext.getGraphicsQueue(), 1, &submitInfo, framesInFlightFences[currentFrame]);
  logger::checkStep("vkQueueSubmit", result);

  VkPresentInfoKHR presentInfo{};
  {
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &vulkanApplicationContext.getSwapchain();
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;
  }

  result = vkQueuePresentKHR(vulkanApplicationContext.getPresentQueue(), &presentInfo);
  logger::checkStep("vkQueuePresentKHR", result);

  // Commented this out for playing around with it later :)
  // vkQueueWaitIdle(context.getPresentQueue());
  currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void Application::prepareGui() {
  ImGui_ImplVulkan_NewFrame();

  // handle the user inputs, the screen resize
  ImGui_ImplGlfw_NewFrame();

  ImGui::NewFrame();

  // ImGui::ShowDemoWindow();
  // ImGui::ShowStackToolWindow();

  ImGui::Begin("little gui ( press TAB to use me )");
  ImGui::Text((std::to_string(static_cast<int>(fps)) + " frames per second").c_str());
  ImGui::Text((std::to_string(static_cast<int>(frameTime * 1000)) + " ms per frame").c_str());

  ImGui::Separator();

  ImGui::Checkbox("Temporal Accumulation", &useTemporal);
  ImGui::Checkbox("A-Trous", &useBlur);

  ImGui::End();
  ImGui::Render();
}

void Application::mainLoop() {
  static float fpsFrameCount = 0, fpsRecordLastTime = 0;

  while (!glfwWindowShouldClose(vulkanApplicationContext.getWindow())) {
    glfwPollEvents();

    prepareGui();

    fpsFrameCount++;
    float currentTime   = static_cast<float>(glfwGetTime());
    deltaTime           = currentTime - frameRecordLastTime;
    frameRecordLastTime = currentTime;

    if (currentTime - fpsRecordLastTime >= fpsUpdateTime) {
      fps               = fpsFrameCount / fpsUpdateTime;
      frameTime         = 1 / fps;
      std::string title = "FPS - " + std::to_string(static_cast<int>(fps));
      glfwSetWindowTitle(vulkanApplicationContext.getWindow(), title.c_str());
      fpsFrameCount = 0;

      fpsRecordLastTime = currentTime;
    }

    camera.processInput(deltaTime);
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
  for (size_t i = 0; i < maxFramesInFlight; i++) {
    vkDestroySemaphore(vulkanApplicationContext.getDevice(), renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(vulkanApplicationContext.getDevice(), imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(vulkanApplicationContext.getDevice(), framesInFlightFences[i], nullptr);
  }

  glfwTerminate();
}