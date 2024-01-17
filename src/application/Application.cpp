#include "application/Application.hpp"

#include "render-context/RenderSystem.hpp"
#include "utils/config/RootDir.h"
#include "utils/incl/Glm.hpp"
#include "utils/logger/Logger.hpp"
#include "window/Window.hpp"

#include "application/app-res/buffers/BuffersHolder.hpp"
#include "application/app-res/images/ImagesHolder.hpp"
#include "application/app-res/models/ModelsHolder.hpp"

#include "gui/gui-manager/ImguiManager.hpp"
#include "utils/fps-sink/FpsSink.hpp"

#include <cassert>
#include <chrono>

// https://www.reddit.com/r/vulkan/comments/10io2l8/is_framesinflight_fif_method_really_worth_it/
static int constexpr kStratumFilterSize = 6;
static int constexpr kATrousSize        = 5;
static int constexpr kFramesInFlight    = 2;

Camera *Application::getCamera() { return _camera.get(); }

Application::Application()
    : _iCap(kATrousSize), _appContext(VulkanApplicationContext::getInstance()) {
  _window = std::make_unique<Window>(WindowStyle::kFullScreen);
  _appContext->init(&_logger, _window->getGlWindow());
  _camera = std::make_unique<Camera>(_window.get());

  _buffersHolder = std::make_unique<BuffersHolder>();
  _imagesHolder  = std::make_unique<ImagesHolder>(_appContext);
  _modelsHolder  = std::make_unique<ModelsHolder>(_appContext, &_logger);
  _imguiManager =
      std::make_unique<ImguiManager>(_appContext, _window.get(), &_logger, kFramesInFlight);
  _fpsSink = std::make_unique<FpsSink>();

  _init();
}

Application::~Application() = default;

void Application::run() {
  _mainLoop();
  _cleanup();
}

void Application::_cleanup() {
  _logger.print("Application is cleaning up resources...");

  for (size_t i = 0; i < kFramesInFlight; i++) {
    vkDestroySemaphore(_appContext->getDevice(), _renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(_appContext->getDevice(), _imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(_appContext->getDevice(), _framesInFlightFences[i], nullptr);
  }

  for (auto &commandBuffer : _commandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }
}

void Application::_createTrisScene() {
  // creates material, loads models from files, creates bvh
  _trisScene = std::make_unique<GpuModel::TrisScene>();
}

void Application::_createSvoScene() { _svoScene = std::make_unique<SvoScene>(&_logger); }

void Application::_updateUbos(size_t currentFrame) {
  static uint32_t currentSample = 0;
  static glm::mat4 lastMvpe{1.0F};

  auto currentTime = static_cast<float>(glfwGetTime());

  GlobalUniformBufferObject globalUbo = {
      _camera->getPosition(),
      _camera->getFront(),
      _camera->getUp(),
      _camera->getRight(),
      _appContext->getSwapchainExtentWidth(),
      _appContext->getSwapchainExtentHeight(),
      _camera->getVFov(),
      currentSample,
      currentTime,
  };
  _buffersHolder->getGlobalBufferBundle()->getBuffer(currentFrame)->fillData(&globalUbo);

  auto thisMvpe =
      _camera->getProjectionMatrix(static_cast<float>(_appContext->getSwapchainExtentWidth()) /
                                   static_cast<float>(_appContext->getSwapchainExtentHeight())) *
      _camera->getViewMatrix();
  GradientProjectionUniformBufferObject gpUbo = {
      static_cast<int>(!_useGradientProjection),
      thisMvpe,
  };
  _buffersHolder->getGradientProjectionBufferBundle()->getBuffer(currentFrame)->fillData(&gpUbo);

  RtxUniformBufferObject rtxUbo = {
      static_cast<uint32_t>(_trisScene->triangles.size()),
      static_cast<uint32_t>(_trisScene->lights.size()),
      static_cast<int>(_movingLightSource),
      _outputType,
      _offsetX,
      _offsetY,
  };

  _buffersHolder->getRtxBufferBundle()->getBuffer(currentFrame)->fillData(&rtxUbo);

  for (int i = 0; i < kStratumFilterSize; i++) {
    StratumFilterUniformBufferObject sfUbo = {i, static_cast<int>(!_useStratumFiltering)};
    _buffersHolder->getStratumFilterBufferBundle(i)->getBuffer(currentFrame)->fillData(&sfUbo);
  }

  TemporalFilterUniformBufferObject tfUbo = {
      static_cast<int>(!_useTemporalBlend),
      static_cast<int>(_useNormalTest),
      _normalThreshold,
      _blendingAlpha,
      lastMvpe,
  };
  _buffersHolder->getTemperalFilterBufferBundle()->getBuffer(currentFrame)->fillData(&tfUbo);
  lastMvpe = thisMvpe;

  VarianceUniformBufferObject varianceUbo = {
      static_cast<int>(!_useVarianceEstimation),
      static_cast<int>(_skipStoppingFunctions),
      static_cast<int>(_useTemporalVariance),
      _varianceKernelSize,
      _variancePhiGaussian,
      _variancePhiDepth,
  };

  _buffersHolder->getVarianceBufferBundle()->getBuffer(currentFrame)->fillData(&varianceUbo);

  for (int i = 0; i < kATrousSize; i++) {
    // update ubo for the sampleDistance
    BlurFilterUniformBufferObject bfUbo = {
        static_cast<int>(!_useATrous),
        i,
        _iCap,
        static_cast<int>(_useVarianceGuidedFiltering),
        static_cast<int>(_useGradientInDepth),
        _phiLuminance,
        _phiDepth,
        _phiNormal,
        static_cast<int>(_ignoreLuminanceAtFirstIteration),
        static_cast<int>(_changingLuminancePhi),
        static_cast<int>(_useJittering),
    };
    _buffersHolder->getBlurFilterBufferBundle(i)->getBuffer(currentFrame)->fillData(&bfUbo);
  }

  PostProcessingUniformBufferObject postProcessingUbo = {_displayType};
  _buffersHolder->getPostProcessingBufferBundle()
      ->getBuffer(currentFrame)
      ->fillData(&postProcessingUbo);

  currentSample++;
}

void Application::_createRenderCommandBuffers() {
  // create command buffers per swapchain image
  _commandBuffers.resize(_appContext->getSwapchainSize()); //  change this later on, because it is
                                                           //  bounded to the swapchain image
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = _appContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

  VkResult result =
      vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, _commandBuffers.data());
  assert(result == VK_SUCCESS && "vkAllocateCommandBuffers failed");

  for (size_t imageIndex = 0; imageIndex < _commandBuffers.size(); imageIndex++) {
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

    // _modelsHolder->getRtxModel()->computeCommand(cmdBuffer, 0, w, h, 1);
    _modelsHolder->getSvoModel()->computeCommand(cmdBuffer, 0, w, h, 1);

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

void Application::_cleanupSwapchainDimensionRelatedResources() {
  _cleanupRenderCommandBuffers();
  // the frame buffer needs to be cleaned up
  _imguiManager->cleanupSwapchainDimensionRelatedResources();
}

void Application::_createSwapchainDimensionRelatedResources() {
  _imagesHolder->onSwapchainResize();
  _modelsHolder->init(_imagesHolder.get(), _buffersHolder.get(), kStratumFilterSize, kATrousSize,
                      kFramesInFlight);

  _createRenderCommandBuffers();
  _imguiManager->createSwapchainDimensionRelatedResources();
}

void Application::_createSemaphoresAndFences() {
  _imageAvailableSemaphores.resize(kFramesInFlight);
  _renderFinishedSemaphores.resize(kFramesInFlight);
  _framesInFlightFences.resize(kFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  // the first call to vkWaitForFences() returns immediately since the fence is
  // already signaled
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < kFramesInFlight; i++) {
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

void Application::_drawFrame() {
  static size_t currentFrame = 0;
  vkWaitForFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);
  vkResetFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame]);

  uint32_t imageIndex = 0;
  // this process is fairly quick, but it is related to communicating with the GPU
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

  _updateUbos(currentFrame);

  _imguiManager->recordGuiCommandBuffer(currentFrame, imageIndex);
  std::vector<VkCommandBuffer> submitCommandBuffers = {
      _commandBuffers[imageIndex], _imguiManager->getCommandBuffer(currentFrame)};

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  // wait until the image is ready
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = &_imageAvailableSemaphores[currentFrame];
  // signal a semaphore after render finished
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = &_renderFinishedSemaphores[currentFrame];
  // wait for no stage
  VkPipelineStageFlags waitStages{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
  submitInfo.pWaitDstStageMask = &waitStages;

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
  currentFrame = (currentFrame + 1) % kFramesInFlight;
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
  static std::chrono::time_point fpsRecordLastTime = std::chrono::steady_clock::now();

  while (glfwWindowShouldClose(_window->getGlWindow()) == 0) {
    glfwPollEvents();

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

      fpsRecordLastTime = std::chrono::steady_clock::now();
      continue;
    }

    auto currentTime  = std::chrono::steady_clock::now();
    auto deltaTime    = currentTime - fpsRecordLastTime;
    fpsRecordLastTime = currentTime;

    double deltaTimeInSec =
        std::chrono::duration<double, std::chrono::seconds::period>(deltaTime).count();
    fpsRecordLastTime = currentTime;

    _fpsSink->addRecord(1.0F / deltaTimeInSec);

    _imguiManager->draw(_fpsSink.get());
    _camera->processInput(deltaTimeInSec);

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
  _createTrisScene();
  _createSvoScene();

  _buffersHolder->init(_trisScene.get(), _svoScene.get(), kStratumFilterSize, kATrousSize,
                       kFramesInFlight);
  _imagesHolder->init();
  _modelsHolder->init(_imagesHolder.get(), _buffersHolder.get(), kStratumFilterSize, kATrousSize,
                      kFramesInFlight);

  _createRenderCommandBuffers();

  _createSemaphoresAndFences();

  // _initGui(); // TODO:

  // attach camera's mouse handler to the window mouse callback, more handlers can be added in the
  // future
  _window->addMouseCallback([this](float mouseDeltaX, float mouseDeltaY) {
    _camera->handleMouseMovement(mouseDeltaX, mouseDeltaY);
  });
}