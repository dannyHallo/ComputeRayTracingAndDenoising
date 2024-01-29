#include "application/Application.hpp"

#include "svo-builder/SvoBuilder.hpp"
#include "svo-tracer/SvoTracer.hpp"

#include "gui/gui-manager/ImguiManager.hpp"
#include "memory/Buffer.hpp"
#include "memory/BufferBundle.hpp"
#include "memory/Image.hpp"
#include "pipelines/ComputePipeline.hpp"
#include "pipelines/DescriptorSetBundle.hpp"
#include "utils/camera/Camera.hpp"
#include "utils/config/RootDir.h"
#include "utils/fps-sink/FpsSink.hpp"
#include "utils/logger/Logger.hpp"
#include "window/Window.hpp"

#include <cassert>
#include <chrono>

// https://www.reddit.com/r/vulkan/comments/10io2l8/is_framesinflight_fif_method_really_worth_it/

static int constexpr kFramesInFlight = 2;

Camera *Application::getCamera() { return _camera.get(); }

Application::Application() : _appContext(VulkanApplicationContext::getInstance()) {
  _window = std::make_unique<Window>(WindowStyle::kFullScreen);
  _appContext->init(&_logger, _window->getGlWindow());
  _camera = std::make_unique<Camera>(_window.get());

  _svoBuilder = std::make_unique<SvoBuilder>(_appContext, &_logger);
  _svoTracer  = std::make_unique<SvoTracer>(_appContext, &_logger, kFramesInFlight, _camera.get());

  _imguiManager = std::make_unique<ImguiManager>(_appContext, _window.get(), &_logger,
                                                 kFramesInFlight, &_svoTracer->getUboData());
  _fpsSink      = std::make_unique<FpsSink>();

  _init();
}

Application::~Application() = default;

void Application::run() {
  _mainLoop();
  _cleanup();
}

void Application::_cleanup() {
  _logger.print("application is cleaning up resources...");

  vkDestroyFence(_appContext->getDevice(), _svoBuildingDoneFence, nullptr);
  for (size_t i = 0; i < kFramesInFlight; i++) {
    vkDestroySemaphore(_appContext->getDevice(), _renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(_appContext->getDevice(), _imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(_appContext->getDevice(), _framesInFlightFences[i], nullptr);
  }
}

void Application::_onSwapchainResize() {
  _appContext->onSwapchainResize();
  _imguiManager->onSwapchainResize();
  _svoTracer->onSwapchainResize();
  _imguiManager->onSwapchainResize();
}

void Application::_createSemaphoresAndFences() {
  _imageAvailableSemaphores.resize(kFramesInFlight);
  _renderFinishedSemaphores.resize(kFramesInFlight);
  _framesInFlightFences.resize(kFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceCreateInfoNotSignalled{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  vkCreateFence(_appContext->getDevice(), &fenceCreateInfoNotSignalled, nullptr,
                &_svoBuildingDoneFence);

  // make sure the fences are ready for the first frames of execution
  VkFenceCreateInfo fenceCreateInfoPreSignalled{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceCreateInfoPreSignalled.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < kFramesInFlight; i++) {
    vkCreateSemaphore(_appContext->getDevice(), &semaphoreInfo, nullptr,
                      &_imageAvailableSemaphores[i]);
    vkCreateSemaphore(_appContext->getDevice(), &semaphoreInfo, nullptr,
                      &_renderFinishedSemaphores[i]);
    vkCreateFence(_appContext->getDevice(), &fenceCreateInfoPreSignalled, nullptr,
                  &_framesInFlightFences[i]);
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

  _svoTracer->updateUboData(currentFrame);

  _imguiManager->recordCommandBuffer(currentFrame, imageIndex);
  // be cautious here! the imageIndex is not the same as the currentFrame
  std::vector<VkCommandBuffer> submitCommandBuffers = {
      _svoTracer->getCommandBuffer(imageIndex), _imguiManager->getCommandBuffer(currentFrame)};

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

  vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo,
                _framesInFlightFences[currentFrame]);

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = &_renderFinishedSemaphores[currentFrame];
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = &_appContext->getSwapchain();
  presentInfo.pImageIndices      = &imageIndex;
  presentInfo.pResults           = nullptr;

  vkQueuePresentKHR(_appContext->getPresentQueue(), &presentInfo);

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

      _onSwapchainResize();

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
  {
    auto startTime = std::chrono::steady_clock::now();
    _svoBuilder->init();
    auto endTime = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration<double, std::chrono::seconds::period>(endTime - startTime).count();
    _logger.print("SVO init time: " + std::to_string(duration) + " seconds");
  }

  _svoTracer->init(_svoBuilder.get());
  _imguiManager->init();

  _createSemaphoresAndFences();

  // attach camera's mouse handler to the window mouse callback, more handlers can be added in the
  // future
  _window->addMouseCallback([this](float mouseDeltaX, float mouseDeltaY) {
    _camera->handleMouseMovement(mouseDeltaX, mouseDeltaY);
  });

  {
    auto startTime = std::chrono::steady_clock::now();
    _svoBuilder->build(_svoBuildingDoneFence);
    vkWaitForFences(_appContext->getDevice(), 1, &_svoBuildingDoneFence, VK_TRUE, UINT64_MAX);
    auto endTime = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration<double, std::chrono::seconds::period>(endTime - startTime).count();
    _logger.print("SVO building time: " + std::to_string(duration) + " seconds");
  }
}