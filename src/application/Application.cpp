#include "application/Application.hpp"

#include "render-context/RenderSystem.hpp"
#include "utils/incl/Glm.hpp"
#include "utils/logger/Logger.hpp"
#include "window/Window.hpp"

#include "application/app-res/buffers/BuffersHolder.hpp"
#include "application/app-res/images/ImagesHolder.hpp"
#include "application/app-res/models/ModelsHolder.hpp"

#include <cassert>

static const int kATrousSize        = 5;
static const int kMaxFramesInFlight = 2;
static const float kFpsUpdateTime   = 0.5F;

std::unique_ptr<Camera> Application::_camera = nullptr;
std::unique_ptr<Window> Application::_window = nullptr;

Camera *Application::getCamera() { return _camera.get(); }

Application::Application() {
  _appContext = VulkanApplicationContext::getInstance();
  _window     = std::make_unique<Window>(WindowStyle::kFullScreen);
  _appContext->init(&_logger, _window->getGlWindow());
  _camera = std::make_unique<Camera>(_window.get());

  _buffersHolder = std::make_unique<BuffersHolder>(_appContext);
  _imagesHolder  = std::make_unique<ImagesHolder>(_appContext);
  _modelsHolder  = std::make_unique<ModelsHolder>(_appContext, &_logger);
}

Application::~Application() = default;

void Application::run() {
  _init();
  _mainLoop();
  _cleanup();
}

void Application::_cleanup() {
  _logger.print("Application is cleaning up resources...");

  for (size_t i = 0; i < kMaxFramesInFlight; i++) {
    vkDestroySemaphore(_appContext->getDevice(), _renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(_appContext->getDevice(), _imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(_appContext->getDevice(), _framesInFlightFences[i], nullptr);
  }

  for (auto &commandBuffer : _commandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }

  for (auto &guiCommandBuffer : _guiCommandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getGuiCommandPool(), 1,
                         &guiCommandBuffer);
  }

  _cleanupFrameBuffers();

  vkDestroyRenderPass(_appContext->getDevice(), _guiPass, nullptr);
  vkDestroyDescriptorPool(_appContext->getDevice(), _guiDescriptorPool, nullptr);

  ImGui_ImplVulkan_Shutdown();
}

void check_vk_result(VkResult resultCode) {
  assert(resultCode == VK_SUCCESS && "check_vk_result failed");
}

void Application::_createScene() {
  // creates material, loads models from files, creates bvh
  _rtScene = std::make_unique<GpuModel::RtScene>();
}

void Application::_updateScene(uint32_t currentImageIndex) {
  static uint32_t currentSample = 0;
  static glm::mat4 lastMvpe{1.0F};

  auto currentTime = static_cast<float>(glfwGetTime());

  GlobalUniformBufferObject globalUbo = {_camera->getPosition(),
                                         _camera->getFront(),
                                         _camera->getUp(),
                                         _camera->getRight(),
                                         _appContext->getSwapchainExtentWidth(),
                                         _appContext->getSwapchainExtentHeight(),
                                         _camera->getVFov(),
                                         currentSample,
                                         currentTime};

  _buffersHolder->getGlobalBuffer(currentImageIndex)->fillData(&globalUbo);

  auto thisMvpe =
      _camera->getProjectionMatrix(static_cast<float>(_appContext->getSwapchainExtentWidth()) /
                                   static_cast<float>(_appContext->getSwapchainExtentHeight())) *
      _camera->getViewMatrix();
  GradientProjectionUniformBufferObject gpUbo = {!_useGradientProjection, thisMvpe};
  _buffersHolder->getGradientProjectionBuffer(currentImageIndex)->fillData(&gpUbo);

  RtxUniformBufferObject rtxUbo = {

      static_cast<uint32_t>(_rtScene->triangles.size()),
      static_cast<uint32_t>(_rtScene->lights.size()),
      _movingLightSource,
      _outputType,
      _offsetX,
      _offsetY};

  _buffersHolder->getRtxBuffer(currentImageIndex)->fillData(&rtxUbo);

  for (int i = 0; i < 6; i++) {
    StratumFilterUniformBufferObject sfUbo = {i, !_useStratumFiltering};
    _buffersHolder->getStratumFilterBuffer(i, currentImageIndex)->fillData(&sfUbo);
  }

  TemporalFilterUniformBufferObject tfUbo = {!_useTemporalBlend, _useNormalTest, _normalThreshold,
                                             _blendingAlpha, lastMvpe};
  _buffersHolder->getTemperalFilterBuffer(currentImageIndex)->fillData(&tfUbo);
  lastMvpe = thisMvpe;

  VarianceUniformBufferObject varianceUbo = {!_useVarianceEstimation, _skipStoppingFunctions,
                                             _useTemporalVariance,    _varianceKernelSize,
                                             _variancePhiGaussian,    _variancePhiDepth};
  _buffersHolder->getVarianceBuffer(currentImageIndex)->fillData(&varianceUbo);

  for (int i = 0; i < kATrousSize; i++) {
    // update ubo for the sampleDistance
    BlurFilterUniformBufferObject bfUbo = {!_useATrous,
                                           i,
                                           _iCap,
                                           _useVarianceGuidedFiltering,
                                           _useGradientInDepth,
                                           _phiLuminance,
                                           _phiDepth,
                                           _phiNormal,
                                           _ignoreLuminanceAtFirstIteration,
                                           _changingLuminancePhi,
                                           _useJittering};
    _buffersHolder->getBlurFilterBuffer(i, currentImageIndex)->fillData(&bfUbo);
  }

  PostProcessingUniformBufferObject postProcessingUbo = {_displayType};
  _buffersHolder->getPostProcessingBuffer(currentImageIndex)->fillData(&postProcessingUbo);

  currentSample++;
}

void Application::_createRenderCommandBuffers() {
  // create command buffers per swapchain image
  _commandBuffers.resize(_appContext->getSwapchainSize());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = _appContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

  VkResult result =
      vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, _commandBuffers.data());
  assert(result == VK_SUCCESS && "vkAllocateCommandBuffers failed");

  for (uint32_t imageIndex = 0; imageIndex < static_cast<uint32_t>(_commandBuffers.size());
       imageIndex++) {
    VkCommandBuffer &cmdBuffer = _commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    assert(result == VK_SUCCESS && "vkBeginCommandBuffer failed");

    _imagesHolder->getTargetImage()->clearImage(cmdBuffer);
    _imagesHolder->getATrousInputImage()->clearImage(cmdBuffer);
    _imagesHolder->getATrousOutputImage()->clearImage(cmdBuffer);
    _imagesHolder->getStratumOffsetImage()->clearImage(cmdBuffer);
    _imagesHolder->getPerStratumLockingImage()->clearImage(cmdBuffer);
    _imagesHolder->getVisibilityImage()->clearImage(cmdBuffer);
    _imagesHolder->getSeedVisibilityImage()->clearImage(cmdBuffer);
    _imagesHolder->getTemporalGradientNormalizationImagePing()->clearImage(cmdBuffer);
    _imagesHolder->getTemporalGradientNormalizationImagePong()->clearImage(cmdBuffer);

    uint32_t w = _appContext->getSwapchainExtentWidth();
    uint32_t h = _appContext->getSwapchainExtentHeight();

    // _modelsHolder->getGradientProjectionModel()->computeCommand(cmdBuffer, imageIndex, w, h,
    // 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    _modelsHolder->getRtxModel()->computeCommand(cmdBuffer, 0, w, h, 1);

    VkMemoryBarrier memoryBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    memoryBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    // _modelsHolder->getGradientModel()->computeCommand(cmdBuffer, imageIndex, w, h, 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    // for (int j = 0; j < 6; j++) {
    //   _modelsHolder->getStratumFilterModel(j)->computeCommand(cmdBuffer, imageIndex, w, h, 1);

    //   vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                        nullptr);
    // }

    // _modelsHolder->getTemporalFilterModel()->computeCommand(cmdBuffer, imageIndex, w, h, 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    // _modelsHolder->getVarianceModel()->computeCommand(cmdBuffer, imageIndex, w, h, 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    // /////////////////////////////////////////////

    // for (int j = 0; j < kATrousSize; j++) {
    //   // dispatch filter shader
    //   _modelsHolder->getATrousModel(j)->computeCommand(cmdBuffer, imageIndex, w, h, 1);
    //   vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                        nullptr);

    //   // copy aTrousImage2 to aTrousImage1 (excluding the last transfer)
    //   if (j != kATrousSize - 1) {
    //     _imagesHolder->getATrousForwardingPair()->forwardCopy(cmdBuffer);
    //   }
    // }

    /////////////////////////////////////////////

    _modelsHolder->getPostProcessingModel()->computeCommand(cmdBuffer, 0, w, h, 1);

    _imagesHolder->getTargetForwardingPair(imageIndex)->forwardCopy(cmdBuffer);

    // copy to history images
    _imagesHolder->getDepthForwardingPair()->forwardCopy(cmdBuffer);
    _imagesHolder->getNormalForwardingPair()->forwardCopy(cmdBuffer);
    _imagesHolder->getGradientForwardingPair()->forwardCopy(cmdBuffer);
    _imagesHolder->getVarianceHistForwardingPair()->forwardCopy(cmdBuffer);
    _imagesHolder->getMeshHashForwardingPair()->forwardCopy(cmdBuffer);

    result = vkEndCommandBuffer(cmdBuffer);
    assert(result == VK_SUCCESS && "vkEndCommandBuffer failed");
  }
}

// these commandbuffers are initially recorded with the models
void Application::_cleanupRenderCommandBuffers() {

  for (auto &commandBuffer : _commandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }
}

void Application::_cleanupFrameBuffers() {
  for (auto &guiFrameBuffer : _guiFrameBuffers) {
    vkDestroyFramebuffer(_appContext->getDevice(), guiFrameBuffer, nullptr);
  }
}

void Application::_cleanupSwapchainDimensionRelatedResources() {
  _cleanupRenderCommandBuffers();
  _cleanupFrameBuffers();
}

void Application::_createSwapchainDimensionRelatedResources() {
  _imagesHolder->onSwapchainResize();
  _modelsHolder->init(_imagesHolder.get(), _buffersHolder.get());

  _createRenderCommandBuffers();
  _createFramebuffers();
}

void Application::_createSemaphoresAndFences() {
  _imageAvailableSemaphores.resize(kMaxFramesInFlight);
  _renderFinishedSemaphores.resize(kMaxFramesInFlight);
  _framesInFlightFences.resize(kMaxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  // the first call to vkWaitForFences() returns immediately since the fence is
  // already signaled
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < kMaxFramesInFlight; i++) {
    VkResult result = vkCreateSemaphore(_appContext->getDevice(), &semaphoreInfo, nullptr,
                                        &_imageAvailableSemaphores[i]);
    assert(result == VK_SUCCESS && "vkCreateSemaphore failed");
    result = vkCreateSemaphore(_appContext->getDevice(), &semaphoreInfo, nullptr,
                               &_renderFinishedSemaphores[i]);
    assert(result == VK_SUCCESS && "vkCreateSemaphore failed");
    result =
        vkCreateFence(_appContext->getDevice(), &fenceInfo, nullptr, &_framesInFlightFences[i]);
    assert(result == VK_SUCCESS && "vkCreateFence failed");
  }
}

void Application::_createGuiCommandBuffers() {
  _guiCommandBuffers.resize(_appContext->getSwapchainSize());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = _appContext->getGuiCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)_guiCommandBuffers.size();

  VkResult result =
      vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, _guiCommandBuffers.data());
  assert(result == VK_SUCCESS && "vkAllocateCommandBuffers failed");
}

void Application::_createGuiRenderPass() {
  // Imgui Pass, right after the main pass
  VkAttachmentDescription attachment = {};
  attachment.format                  = _appContext->getSwapchainImageFormat();
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
      vkCreateRenderPass(_appContext->getDevice(), &renderPassCreateInfo, nullptr, &_guiPass);
  assert(result == VK_SUCCESS && "vkCreateRenderPass failed");
}

void Application::_createFramebuffers() {
  // Create gui frame buffers for gui pass to use
  // Each frame buffer will have an attachment of VkImageView, in this case, the
  // attachments are mSwapchainImageViews
  _guiFrameBuffers.resize(_appContext->getSwapchainSize());

  // Iterate through image views
  for (size_t i = 0; i < _appContext->getSwapchainSize(); i++) {
    VkImageView attachment = _appContext->getSwapchainImageViews()[i];

    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass      = _guiPass;
    frameBufferCreateInfo.attachmentCount = 1;
    frameBufferCreateInfo.pAttachments    = &attachment;
    frameBufferCreateInfo.width           = _appContext->getSwapchainExtentWidth();
    frameBufferCreateInfo.height          = _appContext->getSwapchainExtentHeight();
    frameBufferCreateInfo.layers          = 1;

    VkResult result = vkCreateFramebuffer(_appContext->getDevice(), &frameBufferCreateInfo, nullptr,
                                          &_guiFrameBuffers[i]);
    assert(result == VK_SUCCESS && "vkCreateFramebuffer failed");
  }
}

void Application::_createGuiDescripterPool() {
  std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
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
  // this descriptor pool is created only for once, so we can set the flag to allow individual
  // descriptor sets to be de-allocated
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  // actually imgui takes only 1 descriptor set
  poolInfo.maxSets       = 1000 * poolSizes.size();
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.poolSizeCount = poolSizes.size();

  VkResult result =
      vkCreateDescriptorPool(_appContext->getDevice(), &poolInfo, nullptr, &_guiDescriptorPool);
  assert(result == VK_SUCCESS && "vkCreateDescriptorPool failed");
}

void Application::_initGui() {
  // setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF((kPathToResourceFolder + "/fonts/OverpassMono-Medium.ttf").c_str(),
                               22.0f);

  io.ConfigFlags |= ImGuiWindowFlags_NoNavInputs;

  ImGui::StyleColorsClassic();

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForVulkan(_window->getGlWindow(), true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = _appContext->getVkInstance();
  init_info.PhysicalDevice            = _appContext->getPhysicalDevice();
  init_info.Device                    = _appContext->getDevice();
  init_info.QueueFamily               = _appContext->getQueueFamilyIndices().graphicsFamily;
  init_info.Queue                     = _appContext->getGraphicsQueue();
  init_info.PipelineCache             = VK_NULL_HANDLE;
  init_info.DescriptorPool            = _guiDescriptorPool;
  init_info.Allocator                 = VK_NULL_HANDLE;
  init_info.MinImageCount             = static_cast<uint32_t>(_appContext->getSwapchainSize());
  init_info.ImageCount                = static_cast<uint32_t>(_appContext->getSwapchainSize());
  init_info.CheckVkResultFn           = check_vk_result;
  if (!ImGui_ImplVulkan_Init(&init_info, _guiPass)) {
    _logger.print("failed to init impl");
  }

  // Create fonts texture
  VkCommandBuffer commandBuffer = RenderSystem::beginSingleTimeCommands(
      _appContext->getDevice(), _appContext->getCommandPool());

  if (!ImGui_ImplVulkan_CreateFontsTexture(commandBuffer)) {
    _logger.print("failed to create fonts texture");
  }
  RenderSystem::endSingleTimeCommands(_appContext->getDevice(), _appContext->getCommandPool(),
                                      _appContext->getGraphicsQueue(), commandBuffer);
}

void Application::_recordGuiCommandBuffer(VkCommandBuffer &commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags            = 0;       // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  // A call to vkBeginCommandBuffer will implicitly reset the command buffer
  VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  assert(result == VK_SUCCESS && "vkBeginCommandBuffer failed");

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass            = _guiPass;
  renderPassInfo.framebuffer           = _guiFrameBuffers[imageIndex];
  renderPassInfo.renderArea.extent     = _appContext->getSwapchainExtent();

  VkClearValue clearValue{};
  clearValue.color = {{0.0F, 0.0F, 0.0F, 1.0F}};

  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues    = &clearValue;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Record Imgui Draw Data and draw funcs into command buffer
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  result = vkEndCommandBuffer(commandBuffer);
  assert(result == VK_SUCCESS && "vkEndCommandBuffer failed");
}

void Application::_drawFrame() {
  static size_t currentFrame = 0;
  vkWaitForFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);
  vkResetFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame]);

  uint32_t imageIndex = 0;
  // this process is fairly quick, but it's still better to trigger for semaphores
  // https://stackoverflow.com/questions/60419749/why-does-vkacquirenextimagekhr-never-block-my-thread
  VkResult result =
      vkAcquireNextImageKHR(_appContext->getDevice(), _appContext->getSwapchain(), UINT64_MAX,
                            _imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    // sub-optimal: a swapchain no longer matches the surface properties
    // exactly, but can still be used to present to the surface successfully
    _logger.throwError("resizing is not allowed!");
  }

  _updateScene(imageIndex);

  _recordGuiCommandBuffer(_guiCommandBuffers[imageIndex], imageIndex);
  std::vector<VkCommandBuffer> submitCommandBuffers = {_commandBuffers[imageIndex],
                                                       _guiCommandBuffers[imageIndex]};

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  // wait until the image is ready
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = &_imageAvailableSemaphores[currentFrame];
  // signal a semaphore after render finished
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = &_renderFinishedSemaphores[currentFrame];
  // wait for no stage
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
  submitInfo.pWaitDstStageMask      = static_cast<VkPipelineStageFlags *>(waitStages);

  submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
  submitInfo.pCommandBuffers    = submitCommandBuffers.data();

  result = vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo,
                         _framesInFlightFences[currentFrame]);
  assert(result == VK_SUCCESS && "vkQueueSubmit failed");

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = &_renderFinishedSemaphores[currentFrame];
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = &_appContext->getSwapchain();
  presentInfo.pImageIndices      = &imageIndex;
  presentInfo.pResults           = nullptr;

  result = vkQueuePresentKHR(_appContext->getPresentQueue(), &presentInfo);
  assert(result == VK_SUCCESS && "vkQueuePresentKHR failed");

  // Commented this out for playing around with it later :)
  // vkQueueWaitIdle(context.getPresentQueue());
  currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
}

namespace {
template <std::size_t N>
void comboSelector(const char *comboLabel, const char *(&items)[N], uint32_t &selectedIdx) {
  assert(selectedIdx < N && "selectedIdx is out of range");
  const char *currentSelectedItem = items[selectedIdx];
  if (ImGui::BeginCombo(comboLabel, currentSelectedItem)) {
    for (int n = 0; n < N; n++) {
      bool isSelected = n == selectedIdx;
      if (ImGui::Selectable(items[n], isSelected)) {
        currentSelectedItem = items[n];
        selectedIdx         = n;
      }
      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}
} // namespace

void Application::_prepareGui() {
  ImGui_ImplVulkan_NewFrame();
  // handles the user input, and the resizing of the window
  ImGui_ImplGlfw_NewFrame();

  ImGui::NewFrame();

  ImGui::BeginMainMenuBar();
  if (ImGui::BeginMenu("Config")) {
    ImGui::SeparatorText("Gradient Projection");
    ImGui::Checkbox("Use Gradient Projection", &_useGradientProjection);

    ImGui::SeparatorText("Rtx");
    ImGui::Checkbox("Moving Light Source", &_movingLightSource);
    const char *outputItems[] = {"Combined", "Direct Only", "Indirect Only"};
    comboSelector("Output Type", outputItems, _outputType);
    ImGui::DragFloat("Offset X", &_offsetX, 0.01F, -1.0F, 1.0F);
    ImGui::DragFloat("Offset Y", &_offsetY, 0.01F, -1.0F, 1.0F);

    ImGui::SeparatorText("Stratum Filter");
    ImGui::Checkbox("Use Stratum Filter", &_useStratumFiltering);

    ImGui::SeparatorText("Temporal Blend");
    ImGui::Checkbox("Temporal Accumulation", &_useTemporalBlend);
    ImGui::Checkbox("Use normal test", &_useNormalTest);
    ImGui::SliderFloat("Normal threhold", &_normalThreshold, 0.0F, 1.0F);
    ImGui::SliderFloat("Blending Alpha", &_blendingAlpha, 0.0F, 1.0F);

    ImGui::SeparatorText("Variance Estimation");
    ImGui::Checkbox("Variance Calculation", &_useVarianceEstimation);
    ImGui::Checkbox("Skip Stopping Functions", &_skipStoppingFunctions);
    ImGui::Checkbox("Use Temporal Variance", &_useTemporalVariance);
    ImGui::SliderInt("Variance Kernel Size", &_varianceKernelSize, 1, 15);
    ImGui::SliderFloat("Variance Phi Gaussian", &_variancePhiGaussian, 0.0F, 1.0F);
    ImGui::SliderFloat("Variance Phi Depth", &_variancePhiDepth, 0.0F, 1.0F);

    ImGui::SeparatorText("A-Trous");
    ImGui::Checkbox("A-Trous", &_useATrous);
    ImGui::SliderInt("A-Trous times", &_iCap, 0, 5);
    ImGui::Checkbox("Use variance guided filtering", &_useVarianceGuidedFiltering);
    ImGui::Checkbox("Use gradient in depth", &_useGradientInDepth);
    ImGui::SliderFloat("Luminance Phi", &_phiLuminance, 0.0F, 1.0F);
    ImGui::SliderFloat("Phi Depth", &_phiDepth, 0.0F, 1.0F);
    ImGui::SliderFloat("Phi Normal", &_phiNormal, 0.0F, 200.0F);
    ImGui::Checkbox("Ignore Luminance For First Iteration", &_ignoreLuminanceAtFirstIteration);
    ImGui::Checkbox("Changing luminance phi", &_changingLuminancePhi);
    ImGui::Checkbox("Use jitter", &_useJittering);

    ImGui::SeparatorText("Post Processing");
    const char *displayItems[] = {"Color",      "Variance", "RawCol", "Stratum",
                                  "Visibility", "Gradient", "Custom"};
    comboSelector("Display Type", displayItems, _displayType);

    ImGui::EndMenu();
  }
  ImGui::EndMainMenuBar();

  auto *mainGuiViewPort = ImGui::GetMainViewport();
  // float width           = mainGuiViewPort->Size.x;
  float height = mainGuiViewPort->Size.y;

  const float kStatsWindowWidth  = 200.0F;
  const float kStatsWindowHeight = 80.0F;

  ImGui::SetNextWindowPos(ImVec2(0, height - kStatsWindowHeight));
  ImGui::Begin("Stats", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing |
                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

  ImGui::SetWindowSize(ImVec2(kStatsWindowWidth, kStatsWindowHeight));
  ImGui::Text("fps : %.2f", _fps);
  ImGui::Text("mspf: %.2f", 1 / _fps * 1000.F);

  ImGui::End();
  ImGui::Render();
}

void Application::_waitForTheWindowToBeResumed() {
  int width  = _window->getFrameBufferWidth();
  int height = _window->getFrameBufferHeight();

  while (width == 0 || height == 0) {
    glfwWaitEvents();
    width  = _window->getFrameBufferWidth();
    height = _window->getFrameBufferHeight();
  }
}

void Application::_mainLoop() {
  static float fpsFrameCount     = 0;
  static float fpsRecordLastTime = 0;

  while (glfwWindowShouldClose(_window->getGlWindow()) == 0) {
    glfwPollEvents();
    auto &io = ImGui::GetIO();
    // the MousePos is problematic when the window is not focused, so we set it
    // manually here
    io.MousePos = ImVec2(_window->getCursorXPos(), _window->getCursorYPos());

    if (glfwGetWindowAttrib(_window->getGlWindow(), GLFW_FOCUSED) == GLFW_FALSE) {
      _logger.print("window is not focused");
    }

    if (_window->windowSizeChanged() || _needToToggleWindowStyle()) {
      _window->setWindowSizeChanged(false);

      vkDeviceWaitIdle(_appContext->getDevice());
      _waitForTheWindowToBeResumed();

      _cleanupSwapchainDimensionRelatedResources();
      _appContext->cleanupSwapchainDimensionRelatedResources();
      _appContext->createSwapchainDimensionRelatedResources();
      _createSwapchainDimensionRelatedResources();

      fpsFrameCount     = 0;
      fpsRecordLastTime = 0;
      continue;
    }
    _prepareGui();

    fpsFrameCount++;
    auto currentTime     = static_cast<float>(glfwGetTime());
    _deltaTime           = currentTime - _frameRecordLastTime;
    _frameRecordLastTime = currentTime;

    if (currentTime - fpsRecordLastTime >= kFpsUpdateTime) {
      _fps              = fpsFrameCount / kFpsUpdateTime;
      std::string title = "FPS - " + std::to_string(static_cast<int>(_fps));
      glfwSetWindowTitle(_window->getGlWindow(), title.c_str());
      fpsFrameCount = 0;

      fpsRecordLastTime = currentTime;
    }

    _camera->processInput(_deltaTime);
    _drawFrame();
  }

  vkDeviceWaitIdle(_appContext->getDevice());
}

bool Application::_needToToggleWindowStyle() {
  if (_window->isInputBitActive(GLFW_KEY_F)) {
    _window->disableInputBit(GLFW_KEY_F);
    _window->toggleWindowStyle();
    return true;
  }
  return false;
}

void Application::_init() {
  _createScene();

  _buffersHolder->init(_rtScene.get());
  _imagesHolder->init();
  _modelsHolder->init(_imagesHolder.get(), _buffersHolder.get());

  _createRenderCommandBuffers();
  _createGuiCommandBuffers();

  _createSemaphoresAndFences();

  _createGuiRenderPass();

  _createFramebuffers();

  _createGuiDescripterPool();

  _initGui();

  // attach camera's mouse handler to the window mouse callback, more handlers can be added in the
  // future
  _window->addMouseCallback([](float mouseDeltaX, float mouseDeltaY) {
    _camera->handleMouseMovement(mouseDeltaX, mouseDeltaY);
  });
}