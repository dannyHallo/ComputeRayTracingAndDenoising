#include "Application.h"

Camera camera{};
VulkanApplicationContext vulkanApplicationContext{};

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

Application::Application() {}

void Application::run() {
  initVulkan();
  mainLoop();
  cleanup();
}

static void check_vk_result(VkResult err) {
  if (err == 0)
    return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0)
    abort();
}

void Application::initScene() {
  // = descriptor sets size
  uint32_t swapchainImageViewSize = static_cast<uint32_t>(vulkanApplicationContext.getSwapchainImageViews().size());

  rtScene = std::make_shared<GpuModel::Scene>();

  // uniform buffers are faster to fill compared to storage buffers, but they are restricted in their size
  // Buffer bundle is an array of buffers, one per each swapchain image/descriptor set.
  rtxBufferBundle = std::make_shared<BufferBundle>(swapchainImageViewSize);
  BufferUtils::createBundle<RtxUniformBufferObject>(rtxBufferBundle.get(), RtxUniformBufferObject{},
                                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  temperalFilterBufferBundle = std::make_shared<BufferBundle>(swapchainImageViewSize);
  BufferUtils::createBundle<TemporalFilterUniformBufferObject>(temperalFilterBufferBundle.get(),
                                                               TemporalFilterUniformBufferObject{},
                                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < aTrousSize; i++) {
    auto blurFilterBufferBundle = std::make_shared<BufferBundle>(swapchainImageViewSize);
    BufferUtils::createBundle<BlurFilterUniformBufferObject>(blurFilterBufferBundle.get(), BlurFilterUniformBufferObject{},
                                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    blurFilterBufferBundles.emplace_back(blurFilterBufferBundle);
  }

  auto triangleBufferBundle = std::make_shared<BufferBundle>(swapchainImageViewSize);
  BufferUtils::createBundle<GpuModel::Triangle>(triangleBufferBundle.get(), rtScene->triangles.data(), rtScene->triangles.size(),
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  auto materialBufferBundle = std::make_shared<BufferBundle>(swapchainImageViewSize);
  BufferUtils::createBundle<GpuModel::Material>(materialBufferBundle.get(), rtScene->materials.data(), rtScene->materials.size(),
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  auto aabbBufferBundle = std::make_shared<BufferBundle>(swapchainImageViewSize);
  BufferUtils::createBundle<GpuModel::BvhNode>(aabbBufferBundle.get(), rtScene->bvhNodes.data(), rtScene->bvhNodes.size(),
                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  auto lightsBufferBundle = std::make_shared<BufferBundle>(swapchainImageViewSize);
  BufferUtils::createBundle<GpuModel::Light>(lightsBufferBundle.get(), rtScene->lights.data(), rtScene->lights.size(),
                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  auto spheresBufferBundle = std::make_shared<BufferBundle>(swapchainImageViewSize);
  BufferUtils::createBundle<GpuModel::Sphere>(spheresBufferBundle.get(), rtScene->spheres.data(), rtScene->spheres.size(),
                                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  targetImage = std::make_shared<Image>();
  ImageUtils::createImage(
      vulkanApplicationContext.getSwapchainExtent().width, vulkanApplicationContext.getSwapchainExtent().height, 1,
      VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, targetImage);
  ImageUtils::transitionImageLayout(targetImage->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  rawImage = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                          VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, rawImage);
  ImageUtils::transitionImageLayout(rawImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  blurHImage = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                          VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, blurHImage);
  ImageUtils::transitionImageLayout(blurHImage->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  aTrousImage1 = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                          VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, aTrousImage1);
  ImageUtils::transitionImageLayout(aTrousImage1->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  aTrousImage2 = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                          VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, aTrousImage2);
  ImageUtils::transitionImageLayout(aTrousImage2->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  positionImage = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                          VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, positionImage);
  ImageUtils::transitionImageLayout(positionImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  historySamplesImage = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT,
                          VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, historySamplesImage);
  ImageUtils::transitionImageLayout(historySamplesImage->image, VK_FORMAT_R32_UINT, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  depthImage = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_SFLOAT,
                          VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, depthImage);
  ImageUtils::transitionImageLayout(depthImage->image, VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                    1);

  normalImage = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                          VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, normalImage);
  ImageUtils::transitionImageLayout(normalImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  meshHashImage1 = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT,
                          VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, meshHashImage1);
  ImageUtils::transitionImageLayout(meshHashImage1->image, VK_FORMAT_R32_UINT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                    1);

  meshHashImage2 = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT,
                          VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_GPU_ONLY, meshHashImage2);
  ImageUtils::transitionImageLayout(meshHashImage2->image, VK_FORMAT_R32_UINT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                    1);

  accumulationImage = std::make_shared<Image>();
  ImageUtils::createImage(vulkanApplicationContext.getSwapchainExtent().width,
                          vulkanApplicationContext.getSwapchainExtent().height, 1, VK_SAMPLE_COUNT_1_BIT,
                          VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, accumulationImage);
  ImageUtils::transitionImageLayout(accumulationImage->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_GENERAL, 1);

  // rtx.comp
  auto rtxMat = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/rtx.spv");
  {
    rtxMat->addUniformBufferBundle(rtxBufferBundle, VK_SHADER_STAGE_COMPUTE_BIT);
    // input
    rtxMat->addStorageImage(positionImage, VK_SHADER_STAGE_COMPUTE_BIT);
    rtxMat->addStorageImage(normalImage, VK_SHADER_STAGE_COMPUTE_BIT);
    rtxMat->addStorageImage(depthImage, VK_SHADER_STAGE_COMPUTE_BIT);
    rtxMat->addStorageImage(meshHashImage1, VK_SHADER_STAGE_COMPUTE_BIT);
    // output
    rtxMat->addStorageImage(rawImage, VK_SHADER_STAGE_COMPUTE_BIT);
    // buffers
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
    temporalFilterMat->addStorageImage(depthImage, VK_SHADER_STAGE_COMPUTE_BIT);
    temporalFilterMat->addStorageImage(meshHashImage1, VK_SHADER_STAGE_COMPUTE_BIT);
    temporalFilterMat->addStorageImage(meshHashImage2, VK_SHADER_STAGE_COMPUTE_BIT);
    // output
    temporalFilterMat->addStorageImage(aTrousImage1, VK_SHADER_STAGE_COMPUTE_BIT);
  }
  temporalFilterModel = std::make_shared<ComputeModel>(temporalFilterMat);

  for (int i = 0; i < aTrousSize; i++) {
    auto blurFilterPhase1Mat = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/blurPhase1.spv");
    {
      blurFilterPhase1Mat->addUniformBufferBundle(blurFilterBufferBundles[i], VK_SHADER_STAGE_COMPUTE_BIT);
      // input
      blurFilterPhase1Mat->addStorageImage(aTrousImage1, VK_SHADER_STAGE_COMPUTE_BIT);
      blurFilterPhase1Mat->addStorageImage(normalImage, VK_SHADER_STAGE_COMPUTE_BIT);
      blurFilterPhase1Mat->addStorageImage(depthImage, VK_SHADER_STAGE_COMPUTE_BIT);
      blurFilterPhase1Mat->addStorageImage(positionImage, VK_SHADER_STAGE_COMPUTE_BIT);
      // output
      blurFilterPhase1Mat->addStorageImage(blurHImage, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    blurFilterPhase1Models.emplace_back(std::make_shared<ComputeModel>(blurFilterPhase1Mat));

    auto blurFilterPhase2Mat = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/blurPhase2.spv");
    {
      blurFilterPhase2Mat->addUniformBufferBundle(blurFilterBufferBundles[i], VK_SHADER_STAGE_COMPUTE_BIT);
      // input
      blurFilterPhase2Mat->addStorageImage(aTrousImage1, VK_SHADER_STAGE_COMPUTE_BIT);
      blurFilterPhase2Mat->addStorageImage(blurHImage, VK_SHADER_STAGE_COMPUTE_BIT);
      blurFilterPhase2Mat->addStorageImage(normalImage, VK_SHADER_STAGE_COMPUTE_BIT);
      blurFilterPhase2Mat->addStorageImage(depthImage, VK_SHADER_STAGE_COMPUTE_BIT);
      blurFilterPhase2Mat->addStorageImage(positionImage, VK_SHADER_STAGE_COMPUTE_BIT);
      // output
      blurFilterPhase2Mat->addStorageImage(aTrousImage2, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    blurFilterPhase2Models.emplace_back(std::make_shared<ComputeModel>(blurFilterPhase2Mat));
  }

  auto blurFilterPhase3Mat = std::make_shared<ComputeMaterial>(path_prefix + "/shaders/generated/blurPhase3.spv");
  {
    // input
    blurFilterPhase3Mat->addStorageImage(aTrousImage2, VK_SHADER_STAGE_COMPUTE_BIT);
    blurFilterPhase3Mat->addStorageImage(normalImage, VK_SHADER_STAGE_COMPUTE_BIT);
    blurFilterPhase3Mat->addStorageImage(depthImage, VK_SHADER_STAGE_COMPUTE_BIT);
    blurFilterPhase3Mat->addStorageImage(positionImage, VK_SHADER_STAGE_COMPUTE_BIT);
    // output
    blurFilterPhase3Mat->addStorageImage(targetImage, VK_SHADER_STAGE_COMPUTE_BIT);
  }
  blurFilterPhase3Model = std::make_shared<ComputeModel>(blurFilterPhase3Mat);

  // postProcessScene      = std::make_shared<Scene>(RenderPassType::eFlat);

  // auto screenTex      = std::make_shared<Texture>(targetImage);
  // auto screenMaterial = std::make_shared<Material>(path_prefix + "/shaders/generated/post-process-vert.spv",
  //                                                  path_prefix + "/shaders/generated/post-process-frag.spv");
  // screenMaterial->addTexture(screenTex, VK_SHADER_STAGE_FRAGMENT_BIT);
  // postProcessScene->addModel(std::make_shared<DrawableModel>(screenMaterial, MeshType::ePlane));
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
                                   (uint32_t)rtScene->lights.size(),
                                   (uint32_t)rtScene->spheres.size()};
  {
    auto &allocation = rtxBufferBundle->buffers[currentImage]->allocation;
    void *data;
    vmaMapMemory(vulkanApplicationContext.getAllocator(), allocation, &data);
    memcpy(data, &rtxUbo, sizeof(rtxUbo));
    vmaUnmapMemory(vulkanApplicationContext.getAllocator(), allocation);
  }

  TemporalFilterUniformBufferObject tfUbo = {!useTemporal, lastMvpe, vulkanApplicationContext.getSwapchainExtent().width,
                                             vulkanApplicationContext.getSwapchainExtent().height};
  {
    auto &allocation = temperalFilterBufferBundle->buffers[currentImage]->allocation;
    void *data;
    vmaMapMemory(vulkanApplicationContext.getAllocator(), allocation, &data);
    memcpy(data, &tfUbo, sizeof(tfUbo));
    vmaUnmapMemory(vulkanApplicationContext.getAllocator(), allocation);

    lastMvpe = camera.getProjectionMatrix(static_cast<float>(vulkanApplicationContext.getSwapchainExtent().width) /
                                          static_cast<float>(vulkanApplicationContext.getSwapchainExtent().height)) *
               camera.getViewMatrix();
  }

  for (int j = 0; j < aTrousSize; j++) {
      // update ubo for the sampleDistance
      BlurFilterUniformBufferObject bfUbo = {!useBlur, j};
      {
        auto &allocation = blurFilterBufferBundles[j]->buffers[currentImage]->allocation;
        void *data;
        vmaMapMemory(vulkanApplicationContext.getAllocator(), allocation, &data);
        memcpy(data, &bfUbo, sizeof(bfUbo));
        vmaUnmapMemory(vulkanApplicationContext.getAllocator(), allocation);
      }
  }

  currentSample++;
}

void Application::createRenderCommandBuffers() {
  commandBuffers.resize(vulkanApplicationContext.getSwapchainImageViews().size());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = vulkanApplicationContext.getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(vulkanApplicationContext.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  VkImageMemoryBarrier targetTexTransDst2ReadOnly = ImageUtils::transferDstToReadOnlyBarrier(targetImage->image);
  VkImageMemoryBarrier targetTexReadOnly2General  = ImageUtils::readOnlyToGeneralBarrier(targetImage->image);
  VkImageMemoryBarrier targetTexGeneral2ReadOnly  = ImageUtils::generalToReadOnlyBarrier(targetImage->image);
  VkImageMemoryBarrier targetTexGeneral2TransDst  = ImageUtils::generalToTransferDstBarrier(targetImage->image);
  VkImageMemoryBarrier targetTexGeneral2TransSrc  = ImageUtils::generalToTransferSrcBarrier(targetImage->image);
  VkImageMemoryBarrier targetTexTransSrc2General  = ImageUtils::transferSrcToGeneralBarrier(targetImage->image);
  VkImageMemoryBarrier targetTexTransDst2General  = ImageUtils::transferDstToGeneralBarrier(targetImage->image);

  VkImageMemoryBarrier accumTexGeneral2TransDst = ImageUtils::generalToTransferDstBarrier(accumulationImage->image);
  VkImageMemoryBarrier accumTexTransDst2General = ImageUtils::transferDstToGeneralBarrier(accumulationImage->image);

  VkImageMemoryBarrier aTrousTex1General2TransDst = ImageUtils::generalToTransferDstBarrier(aTrousImage1->image);
  VkImageMemoryBarrier aTrousTex1TransDst2General = ImageUtils::transferDstToGeneralBarrier(aTrousImage1->image);
  VkImageMemoryBarrier aTrousTex2General2TransSrc = ImageUtils::generalToTransferSrcBarrier(aTrousImage2->image);
  VkImageMemoryBarrier aTrousTex2TransSrc2General = ImageUtils::transferSrcToGeneralBarrier(aTrousImage2->image);

  VkImageMemoryBarrier triIdTex1General2TransSrc = ImageUtils::generalToTransferSrcBarrier(meshHashImage1->image);
  VkImageMemoryBarrier triIdTex1TransSrc2General = ImageUtils::transferSrcToGeneralBarrier(meshHashImage1->image);
  VkImageMemoryBarrier triIdTex2General2TransDst = ImageUtils::generalToTransferDstBarrier(meshHashImage2->image);
  VkImageMemoryBarrier triIdTex2TransDst2General = ImageUtils::transferDstToGeneralBarrier(meshHashImage2->image);

  VkImageCopy imgCopyRegion = ImageUtils::imageCopyRegion(vulkanApplicationContext.getSwapchainExtent().width,
                                                          vulkanApplicationContext.getSwapchainExtent().height);

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

    if (vkBeginCommandBuffer(currentCommandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkClearColorValue clearColor{0, 0, 0, 0};
    VkImageSubresourceRange clearRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(currentCommandBuffer, targetImage->image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
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

    for (int j = 0; j < aTrousSize; j++) {
      // update ubo for the sampleDistance
      BlurFilterUniformBufferObject bfUbo = {!useBlur, j};
      {
        auto &allocation = blurFilterBufferBundles[j]->buffers[i]->allocation;
        void *data;
        vmaMapMemory(vulkanApplicationContext.getAllocator(), allocation, &data);
        memcpy(data, &bfUbo, sizeof(bfUbo));
        vmaUnmapMemory(vulkanApplicationContext.getAllocator(), allocation);
      }

      // dispatch 2 stages of separate shaders
      blurFilterPhase1Models[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->width / 32,
                                                targetImage->height / 32, 1);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                           nullptr, 0, nullptr, 0, nullptr);
      blurFilterPhase2Models[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->width / 32,
                                                targetImage->height / 32, 1);

      // accum texture when the first filter is done
      // (from SVGF) accumulation image should use a properly smoothed image,
      // the presented one is not ideal due to the over-blur over time
      if (j == 0) {
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &aTrousTex2General2TransSrc);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &accumTexGeneral2TransDst);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &aTrousTex1General2TransDst);
        vkCmdCopyImage(currentCommandBuffer, aTrousImage2->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, aTrousImage1->image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdCopyImage(currentCommandBuffer, aTrousImage2->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, accumulationImage->image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &aTrousTex1TransDst2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &aTrousTex2TransSrc2General);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &accumTexTransDst2General);
      }
      // copy aTrousImage2 to aTrousImage1 (excluding the last transfer)
      else if (j != aTrousSize - 1) {
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
    }

    /////////////////////////////////////////////

    blurFilterPhase3Model->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i), targetImage->width / 32,
                                          targetImage->height / 32, 1);

    // vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
    // 0,
    //                      nullptr, 0, nullptr, 1, &targetTexGeneral2ReadOnly);

    // copy targetTex to swapchainImage
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &targetTexGeneral2TransSrc);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &swapchainImageUndefined2TransferDst);
    vkCmdCopyImage(currentCommandBuffer, targetImage->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   vulkanApplicationContext.getSwapchainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &targetTexTransSrc2General);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &swapchainImageTransferDst2ColorAttachment);

    // sync mesh hash image for next frame
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &triIdTex1General2TransSrc);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &triIdTex2General2TransDst);
    vkCmdCopyImage(currentCommandBuffer, meshHashImage1->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, meshHashImage2->image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &triIdTex1TransSrc2General);
    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &triIdTex2TransDst2General);

    // Bind graphics pipeline and dispatch draw command.
    // postProcessScene->writeRenderCommand(currentCommandBuffer, i);

    // vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    // 0,
    //                      0, nullptr, 0, nullptr, 1, &targetTexReadOnly2General);

    if (vkEndCommandBuffer(currentCommandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
}

void Application::createSyncObjects() {
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
    if (vkCreateSemaphore(vulkanApplicationContext.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
            VK_SUCCESS ||
        vkCreateSemaphore(vulkanApplicationContext.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
            VK_SUCCESS ||
        vkCreateFence(vulkanApplicationContext.getDevice(), &fenceInfo, nullptr, &framesInFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
  }
}

void Application::createGuiCommandBuffers() {
  guiCommandBuffers.resize(vulkanApplicationContext.getSwapchainImageViews().size());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = vulkanApplicationContext.getGuiCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)guiCommandBuffers.size();

  if (vkAllocateCommandBuffers(vulkanApplicationContext.getDevice(), &allocInfo, guiCommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate gui command buffers!");
  }
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
  if (vkCreateRenderPass(vulkanApplicationContext.getDevice(), &renderPassCreateInfo, nullptr, &imGuiPass) != VK_SUCCESS) {
    throw std::runtime_error("Could not create Dear ImGui's render pass");
  }
}

void Application::createGuiFramebuffers() {
  // Create gui frame buffers for gui pass to use
  // Each frame buffer will have an attachment of VkImageView, in this case, the attachments are
  // mSwapchainImageViews
  swapchainGuiFrameBuffers.resize(vulkanApplicationContext.getSwapchainImageViews().size());

  // Iterate through image views
  for (size_t i = 0; i < vulkanApplicationContext.getSwapchainImageViews().size(); i++) {
    VkImageView attachment = vulkanApplicationContext.getSwapchainImageViews()[i];

    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass      = imGuiPass;
    frameBufferCreateInfo.attachmentCount = 1;
    frameBufferCreateInfo.pAttachments    = &attachment;
    frameBufferCreateInfo.width           = vulkanApplicationContext.getSwapchainExtent().width;
    frameBufferCreateInfo.height          = vulkanApplicationContext.getSwapchainExtent().height;
    frameBufferCreateInfo.layers          = 1;

    if (vkCreateFramebuffer(vulkanApplicationContext.getDevice(), &frameBufferCreateInfo, nullptr,
                            &swapchainGuiFrameBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
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

  if (vkCreateDescriptorPool(vulkanApplicationContext.getDevice(), &poolInfo, nullptr, &guiDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

VkCommandBuffer Application::beginSingleTimeCommands() {
  VkCommandBuffer commandBuffer;

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool        = vulkanApplicationContext.getCommandPool();
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(vulkanApplicationContext.getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
    print("failed to allocate command buffers!");

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

  vkFreeCommandBuffers(vulkanApplicationContext.getDevice(), vulkanApplicationContext.getCommandPool(), 1, &commandBuffer);
}

void Application::initGui() {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  // Does nothing, avoid compiler warning
  (void)io;

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

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
  init_info.MinImageCount             = vulkanApplicationContext.getSwapchainImageViews().size();
  init_info.ImageCount                = vulkanApplicationContext.getSwapchainImageViews().size();
  init_info.CheckVkResultFn           = check_vk_result;
  if (!ImGui_ImplVulkan_Init(&init_info, imGuiPass)) {
    print("failed to init impl");
  }

  // Create fonts texture
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();
  if (!ImGui_ImplVulkan_CreateFontsTexture(commandBuffer))
    print("failed to create fonts texture");
  endSingleTimeCommands(commandBuffer);
}

void Application::recordGuiCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags            = 0;       // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  // A call to vkBeginCommandBuffer will implicitly reset the command buffer
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

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
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void Application::drawFrame() {
  static size_t currentFrame = 0;
  vkWaitForFences(vulkanApplicationContext.getDevice(), 1, &framesInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(vulkanApplicationContext.getDevice(), 1, &framesInFlightFences[currentFrame]);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(vulkanApplicationContext.getDevice(), vulkanApplicationContext.getSwapchain(),
                                          UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    // sub-optimal: a swapchain no longer matches the surface properties exactly, but can still be used to present
    // to the surface successfully
    throw std::runtime_error("failed to acquire swap chain image!");
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

  if (vkQueueSubmit(vulkanApplicationContext.getGraphicsQueue(), 1, &submitInfo, framesInFlightFences[currentFrame]) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

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

  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  // Commented this out for playing around with it later :)
  // vkQueueWaitIdle(context.getPresentQueue());
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::prepareGui() {
  ImGui_ImplVulkan_NewFrame();

  // handle the user inputs, the screen resize
  ImGui_ImplGlfw_NewFrame();

  bool openFpsOverlay = true;

  ImGui::NewFrame();

  // ImGui::ShowDemoWindow();
  // ImGui::ShowStackToolWindow();

  ImGui::Begin("little gui");
  // ImGui::Text("Test text XD");

  ImGui::Checkbox("Temporal Accumulation", &useTemporal);
  ImGui::Checkbox("A-Trous", &useBlur);

  // ImGui::Text("Cursor sensitivity:");
  // ImGui::SliderFloat("##cursor_sensitivity", &camera.mCursorSensitivity, 30.0f, 60.0f);

  // ImGui::Text("Base Movement speed:");
  // ImGui::SliderFloat("##base_movement_speed", &camera.mBaseMovementSpeed, 0.0f, 6.0f);

  // ImGui::Text("Top Movement speed:");
  // ImGui::SliderFloat("##top_movement_speed", &camera.mTopMovementSpeed, 2.0f, 6.0f);

  // ImGui::Text("Speed Gain:");
  // ImGui::SliderFloat("##accleration_mult", &camera.mSpeedGain, 0.0f, 0.1f);

  // ImGui::Text("Sun ambient:");
  // ImGui::SliderFloat("##sun_ambient", &sAmbientLight, 0.0f, 1.0f);

  // ImGui::Text("Sun position:");
  // ImGui::SliderFloat3("##sun_position", sSunPos, 0.0f, 1.0f);

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
      std::string title = "FPS - " + std::to_string(static_cast<int>(fpsFrameCount / fpsUpdateTime));
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
  camera.init(&vulkanApplicationContext.getWindowClass());
  initScene();
  createRenderCommandBuffers();
  createGuiCommandBuffers();
  createSyncObjects();
  createGuiRenderPass();
  createGuiFramebuffers();
  createGuiDescripterPool();
  initGui();

  glfwSetCursorPosCallback(vulkanApplicationContext.getWindow(), mouseCallback);
}

void Application::cleanup() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(vulkanApplicationContext.getDevice(), renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(vulkanApplicationContext.getDevice(), imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(vulkanApplicationContext.getDevice(), framesInFlightFences[i], nullptr);
  }

  glfwTerminate();
}