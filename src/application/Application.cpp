#include "application/Application.hpp"

#include "svo-builder/SvoBuilder.hpp"
#include "svo-tracer/SvoTracer.hpp"

#include "camera/Camera.hpp"
#include "file-watcher/ShaderChangeListener.hpp"
#include "imgui-manager/gui-manager/ImguiManager.hpp"
#include "utils/event-dispatcher/GlobalEventDispatcher.hpp"
#include "utils/fps-sink/FpsSink.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"
#include "utils/toml-config/TomlConfigReader.hpp"
#include "window/Window.hpp"

#include <chrono>

// https://www.reddit.com/r/vulkan/comments/10io2l8/is_framesinflight_fif_method_really_worth_it/

Camera *Application::getCamera() { return _camera.get(); }

Application::Application(Logger *logger)
    : _appContext(VulkanApplicationContext::getInstance()), _logger(logger),
      _shaderCompiler(std::make_unique<ShaderCompiler>(logger)),
      _shaderFileWatchListener(std::make_unique<ShaderChangeListener>(_logger)),
      _tomlConfigReader(std::make_unique<TomlConfigReader>(_logger)) {
  _loadConfig();

  _window = std::make_unique<Window>(WindowStyle::kMaximized);
  _appContext->init(_logger, _window->getGlWindow());
  _camera = std::make_unique<Camera>(_window.get());

  _svoBuilder =
      std::make_unique<SvoBuilder>(_appContext, _logger, _shaderCompiler.get(),
                                   _shaderFileWatchListener.get(), _tomlConfigReader.get());
  _svoTracer = std::make_unique<SvoTracer>(_appContext, _logger, _framesInFlight, _camera.get(),
                                           _shaderCompiler.get(), _shaderFileWatchListener.get(),
                                           _tomlConfigReader.get());

  _imguiManager =
      std::make_unique<ImguiManager>(_appContext, _window.get(), _logger, _tomlConfigReader.get(),
                                     _framesInFlight, &_svoTracer->getUboData());
  _fpsSink = std::make_unique<FpsSink>();

  _init();

  GlobalEventDispatcher::get()
      .sink<E_RenderLoopBlockRequest>()
      .connect<&Application::_onRenderLoopBlockRequest>(this);
}

Application::~Application() { GlobalEventDispatcher::get().disconnect<>(this); }

void Application::_loadConfig() {
  _framesInFlight = _tomlConfigReader->getConfig<uint32_t>("Application.framesInFlight");
}

void Application::run() {
  _mainLoop();
  _cleanup();
}

void Application::_cleanup() {
  _logger->info("application is cleaning up resources...");

  for (size_t i = 0; i < _framesInFlight; i++) {
    vkDestroySemaphore(_appContext->getDevice(), _renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(_appContext->getDevice(), _imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(_appContext->getDevice(), _framesInFlightFences[i], nullptr);
  }
}

void Application::_onRenderLoopBlockRequest(E_RenderLoopBlockRequest const &event) {
  _blockState = event.rebuildSvo ? BlockState::kBlockedAndNeedToRebuildSvo : BlockState::kBlocked;
}

void Application::_onSwapchainResize() {
  _appContext->onSwapchainResize();
  _imguiManager->onSwapchainResize();
  _svoTracer->onSwapchainResize();
  _imguiManager->onSwapchainResize();
}

void Application::_createSemaphoresAndFences() {
  _imageAvailableSemaphores.resize(_framesInFlight);
  _renderFinishedSemaphores.resize(_framesInFlight);
  _framesInFlightFences.resize(_framesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceCreateInfoPreSignalled{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceCreateInfoPreSignalled.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < _framesInFlight; i++) {
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
    _logger->error("resizing is not allowed!");
  }

  if (_lastMouseInfo.lmbPressed) {
    auto outputInfo = _svoTracer->getOutputInfo();
    if (!outputInfo.midRayHit) {
      _logger->info("mid ray didn't hit anything");
    } else {
      _logger->info("mid ray hit at: " + std::to_string(outputInfo.midRayHitPos.x) + ", " +
                    std::to_string(outputInfo.midRayHitPos.y) + ", " +
                    std::to_string(outputInfo.midRayHitPos.z));
    }
  }

  _svoTracer->updateUboData(currentFrame);

  _imguiManager->recordCommandBuffer(currentFrame, imageIndex);
  std::vector<VkCommandBuffer> submitCommandBuffers = {
      _svoTracer->getTracingCommandBuffer(currentFrame),
      _svoTracer->getDeliveryCommandBuffer(imageIndex),
      _imguiManager->getCommandBuffer(currentFrame),
  };

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
  currentFrame = (currentFrame + 1) % _framesInFlight;
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

    if (_blockState != BlockState::kUnblocked) {
      vkDeviceWaitIdle(_appContext->getDevice());
      GlobalEventDispatcher::get().trigger<E_RenderLoopBlocked>();
      // then some rebuilding will happen

      if (_blockState == BlockState::kBlockedAndNeedToRebuildSvo) {
        _buildScene();
      }
      _blockState = BlockState::kUnblocked;
    }

    glfwPollEvents();

    if (_window->windowSizeChanged() || _needToToggleWindowStyle()) {
      vkDeviceWaitIdle(_appContext->getDevice());

      _window->setWindowSizeChanged(false);
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
    _logger->info("SVO init time: " + std::to_string(duration) + " seconds");
  }

  _svoTracer->init(_svoBuilder.get());
  _imguiManager->init();

  _createSemaphoresAndFences();

  // attach camera's mouse handler to the window mouse callback
  _window->addMouseCallback(
      [this](MouseInfo const &mouseInfo) { _camera->handleMouseMovement(mouseInfo); });

  _window->addMouseCallback([this](MouseInfo const &mouseInfo) { _syncMouseInfo(mouseInfo); });

  _buildScene();
}

void Application::_syncMouseInfo(MouseInfo const &mouseInfo) { _lastMouseInfo = mouseInfo; }

void Application::_buildScene() {
  auto startTime = std::chrono::steady_clock::now();
  _svoBuilder->buildScene();
  auto endTime = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration<double, std::chrono::seconds::period>(endTime - startTime).count();
  _logger->info("svo building time: " + std::to_string(duration) + " seconds");
}