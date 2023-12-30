#include "Application.hpp"

#include "render-context/RenderSystem.hpp"
#include "scene/ComputeMaterial.hpp"
#include "utils/Logger.hpp"
#include "window/Window.hpp"

#include <cassert>

static const int kATrousSize                   = 5;
static const int kMaxFramesInFlight            = 2;
static const std::string kPathToResourceFolder = std::string(ROOT_DIR) + "resources/";
static const float kFpsUpdateTime              = 0.5F;

std::unique_ptr<Camera> Application::_camera = nullptr;
std::unique_ptr<Window> Application::_window = nullptr;

Camera *Application::getCamera() { return _camera.get(); }

Application::Application() {
  _window     = std::make_unique<Window>(WindowStyle::kFullScreen);
  _appContext = VulkanApplicationContext::getInstance();
  _appContext->init(&_logger, _window->getGlWindow());
  _camera = std::make_unique<Camera>(_window.get());
}

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

  for (auto &mGuiCommandBuffers : _guiCommandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getGuiCommandPool(), 1,
                         &mGuiCommandBuffers);
  }

  _cleanupGuiFrameBuffers();

  vkDestroyRenderPass(_appContext->getDevice(), _guiPass, nullptr);
  vkDestroyDescriptorPool(_appContext->getDevice(), _guiDescriptorPool, nullptr);

  ImGui_ImplVulkan_Shutdown();
}

void check_vk_result(VkResult resultCode) {
  assert(resultCode == VK_SUCCESS && "check_vk_result failed");
}

void Application::_createScene() {
  // creates material, loads models from files, creates bvh
  _rtScene = std::make_unique<GpuModel::Scene>();
}

void Application::_createBufferBundles() {
  // equals to descriptor sets size
  auto swapchainSize = static_cast<uint32_t>(_appContext->getSwapchainSize());
  // uniform buffers are faster to fill compared to storage buffers, but they
  // are restricted in their size Buffer bundle is an array of buffers, one per
  // each swapchain image/descriptor set.
  _globalBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GlobalUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  _gradientProjectionBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GradientProjectionUniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  _rtxBufferBundle = std::make_unique<BufferBundle>(swapchainSize, sizeof(RtxUniformBufferObject),
                                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < 6; i++) {
    auto stratumFilterBufferBundle = std::make_unique<BufferBundle>(
        swapchainSize, sizeof(StratumFilterUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    _stratumFilterBufferBundle.emplace_back(std::move(stratumFilterBufferBundle));
  }

  _temperalFilterBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(TemporalFilterUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  _varianceBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(VarianceUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < kATrousSize; i++) {
    auto blurFilterBufferBundle = std::make_unique<BufferBundle>(
        swapchainSize, sizeof(BlurFilterUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    _blurFilterBufferBundles.emplace_back(std::move(blurFilterBufferBundle));
  }

  _postProcessingBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(PostProcessingUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  _triangleBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Triangle) * _rtScene->triangles.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, _rtScene->triangles.data());

  _materialBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Material) * _rtScene->materials.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, _rtScene->materials.data());

  _bvhBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::BvhNode) * _rtScene->bvhNodes.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, _rtScene->bvhNodes.data());

  _lightsBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Light) * _rtScene->lights.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, _rtScene->lights.data());
}

void Application::_createImagesAndForwardingPairs() {

  {
    std::vector<std::string> filenames{};
    for (int i = 0; i < 64; i++) {
      filenames.emplace_back(kPathToResourceFolder +
                             "/textures/stbn/vec2_2d_1d/"
                             "stbn_vec2_2Dx1D_128x128x64_" +
                             std::to_string(i) + ".png");
    }
    _vec2BlueNoise = std::make_unique<Image>(filenames, VK_IMAGE_USAGE_STORAGE_BIT |
                                                            VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  }
  {
    std::vector<std::string> filenames{};
    for (int i = 0; i < 64; i++) {
      filenames.emplace_back(kPathToResourceFolder +
                             "/textures/stbn/unitvec3_cosine_2d_1d/"
                             "stbn_unitvec3_cosine_2Dx1D_128x128x64_" +
                             std::to_string(i) + ".png");
    }
    _weightedCosineBlueNoise = std::make_unique<Image>(
        filenames, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  }

  auto imageWidth  = _appContext->getSwapchainExtentWidth();
  auto imageHeight = _appContext->getSwapchainExtentHeight();

  _targetImage = std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R8G8B8A8_UNORM,
                                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  // creating forwarding pairs to copy the image result each frame to a specific swapchain
  _targetForwardingPairs.clear();
  for (int i = 0; i < _appContext->getSwapchainSize(); i++) {
    _targetForwardingPairs.emplace_back(std::make_unique<ImageForwardingPair>(
        _targetImage->getVkImage(), _appContext->getSwapchainImages()[i], VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  }

  _rawImage = std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
                                      VK_IMAGE_USAGE_STORAGE_BIT);

  _seedImage = std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_UINT,
                                       VK_IMAGE_USAGE_STORAGE_BIT);

  _aTrousInputImage =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _aTrousOutputImage =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _aTrousForwardingPair = std::make_unique<ImageForwardingPair>(
      _aTrousOutputImage->getVkImage(), _aTrousInputImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _positionImage = std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
                                           VK_IMAGE_USAGE_STORAGE_BIT);

  _depthImage =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _depthImagePrev =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _depthForwardingPair = std::make_unique<ImageForwardingPair>(
      _depthImage->getVkImage(), _depthImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _normalImage =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _normalImagePrev =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _normalForwardingPair = std::make_unique<ImageForwardingPair>(
      _normalImage->getVkImage(), _normalImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _gradientImage =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _gradientImagePrev =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _gradientForwardingPair = std::make_unique<ImageForwardingPair>(
      _gradientImage->getVkImage(), _gradientImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _varianceHistImage =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _varianceHistImagePrev =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _varianceHistForwardingPair = std::make_unique<ImageForwardingPair>(
      _varianceHistImage->getVkImage(), _varianceHistImagePrev->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  _meshHashImage =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _meshHashImagePrev =
      std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _meshHashForwardingPair = std::make_unique<ImageForwardingPair>(
      _meshHashImage->getVkImage(), _meshHashImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _varianceImage = std::make_unique<Image>(imageWidth, imageHeight, VK_FORMAT_R32_SFLOAT,
                                           VK_IMAGE_USAGE_STORAGE_BIT);

  _lastFrameAccumImage = std::make_unique<Image>(
      imageWidth, imageHeight, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  uint32_t perStratumImageWidth  = ceil(static_cast<float>(imageWidth) / 3.0F);
  uint32_t perStratumImageHeight = ceil(static_cast<float>(imageHeight) / 3.0F);
  _stratumOffsetImage =
      std::make_unique<Image>(perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _perStratumLockingImage =
      std::make_unique<Image>(perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32_UINT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _visibilityImage = std::make_unique<Image>(
      perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _seedVisibilityImage = std::make_unique<Image>(
      perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_UINT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _temporalGradientNormalizationImagePing = std::make_unique<Image>(
      perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _temporalGradientNormalizationImagePong = std::make_unique<Image>(
      perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}

void Application::_createComputeModels() {
  auto gradientProjectionMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/gradientProjection.spv");
  {
    gradientProjectionMat->addUniformBufferBundle(_globalBufferBundle.get());
    gradientProjectionMat->addUniformBufferBundle(_gradientProjectionBufferBundle.get());
    // read
    gradientProjectionMat->addStorageImage(_vec2BlueNoise.get());
    gradientProjectionMat->addStorageImage(_weightedCosineBlueNoise.get());
    gradientProjectionMat->addStorageImage(_rawImage.get());
    gradientProjectionMat->addStorageImage(_positionImage.get());
    gradientProjectionMat->addStorageImage(_seedImage.get());
    // write
    gradientProjectionMat->addStorageImage(_stratumOffsetImage.get());
    // (atomic) readwrite
    gradientProjectionMat->addStorageImage(_perStratumLockingImage.get());
    // write
    gradientProjectionMat->addStorageImage(_visibilityImage.get());
    gradientProjectionMat->addStorageImage(_seedVisibilityImage.get());
  }
  _gradientProjectionModel =
      std::make_unique<ComputeModel>(&_logger, std::move(gradientProjectionMat));

  auto rtxMat =
      std::make_unique<ComputeMaterial>(kPathToResourceFolder + "/shaders/generated/rtx.spv");
  {
    rtxMat->addUniformBufferBundle(_globalBufferBundle.get());
    rtxMat->addUniformBufferBundle(_rtxBufferBundle.get());
    // read
    rtxMat->addStorageImage(_vec2BlueNoise.get());
    rtxMat->addStorageImage(_weightedCosineBlueNoise.get());
    rtxMat->addStorageImage(_stratumOffsetImage.get());
    rtxMat->addStorageImage(_visibilityImage.get());
    rtxMat->addStorageImage(_seedVisibilityImage.get());
    // output
    rtxMat->addStorageImage(_positionImage.get());
    rtxMat->addStorageImage(_normalImage.get());
    rtxMat->addStorageImage(_depthImage.get());
    rtxMat->addStorageImage(_meshHashImage.get());
    rtxMat->addStorageImage(_rawImage.get());
    rtxMat->addStorageImage(_seedImage.get());
    rtxMat->addStorageImage(_temporalGradientNormalizationImagePing.get());
    // buffers
    rtxMat->addStorageBufferBundle(_triangleBufferBundle.get());
    rtxMat->addStorageBufferBundle(_materialBufferBundle.get());
    rtxMat->addStorageBufferBundle(_bvhBufferBundle.get());
    rtxMat->addStorageBufferBundle(_lightsBufferBundle.get());
  }
  _rtxModel = std::make_unique<ComputeModel>(&_logger, std::move(rtxMat));

  auto gradientMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/screenSpaceGradient.spv");
  {
    gradientMat->addUniformBufferBundle(_globalBufferBundle.get());
    // i2nput
    gradientMat->addStorageImage(_depthImage.get());
    // output
    gradientMat->addStorageImage(_gradientImage.get());
  }
  _gradientModel = std::make_unique<ComputeModel>(&_logger, std::move(gradientMat));

  _stratumFilterModels.clear();
  for (int i = 0; i < 6; i++) {
    auto stratumFilterMat = std::make_unique<ComputeMaterial>(
        kPathToResourceFolder + "/shaders/generated/stratumFilter.spv");
    {
      stratumFilterMat->addUniformBufferBundle(_globalBufferBundle.get());
      stratumFilterMat->addUniformBufferBundle(_stratumFilterBufferBundle[i].get());
      // input
      stratumFilterMat->addStorageImage(_positionImage.get());
      stratumFilterMat->addStorageImage(_normalImage.get());
      stratumFilterMat->addStorageImage(_depthImage.get());
      stratumFilterMat->addStorageImage(_gradientImage.get());
      stratumFilterMat->addStorageImage(_rawImage.get());
      stratumFilterMat->addStorageImage(_stratumOffsetImage.get());
      // pingpong
      stratumFilterMat->addStorageImage(_temporalGradientNormalizationImagePing.get());
      stratumFilterMat->addStorageImage(_temporalGradientNormalizationImagePong.get());
    }
    _stratumFilterModels.emplace_back(
        std::make_unique<ComputeModel>(&_logger, std::move(stratumFilterMat)));
  }

  auto temporalFilterMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/temporalFilter.spv");
  {
    temporalFilterMat->addUniformBufferBundle(_globalBufferBundle.get());
    temporalFilterMat->addUniformBufferBundle(_temperalFilterBufferBundle.get());
    // input
    temporalFilterMat->addStorageImage(_positionImage.get());
    temporalFilterMat->addStorageImage(_rawImage.get());
    temporalFilterMat->addStorageImage(_depthImage.get());
    temporalFilterMat->addStorageImage(_depthImagePrev.get());
    temporalFilterMat->addStorageImage(_normalImage.get());
    temporalFilterMat->addStorageImage(_normalImagePrev.get());
    temporalFilterMat->addStorageImage(_gradientImage.get());
    temporalFilterMat->addStorageImage(_gradientImagePrev.get());
    temporalFilterMat->addStorageImage(_meshHashImage.get());
    temporalFilterMat->addStorageImage(_meshHashImagePrev.get());
    temporalFilterMat->addStorageImage(_lastFrameAccumImage.get());
    temporalFilterMat->addStorageImage(_varianceHistImagePrev.get());
    // output
    temporalFilterMat->addStorageImage(_aTrousInputImage.get());
    temporalFilterMat->addStorageImage(_varianceHistImage.get());
  }
  _temporalFilterModel = std::make_unique<ComputeModel>(&_logger, std::move(temporalFilterMat));

  auto varianceMat =
      std::make_unique<ComputeMaterial>(kPathToResourceFolder + "/shaders/generated/variance.spv");
  {
    varianceMat->addUniformBufferBundle(_globalBufferBundle.get());
    varianceMat->addUniformBufferBundle(_varianceBufferBundle.get());
    // input
    varianceMat->addStorageImage(_aTrousInputImage.get());
    varianceMat->addStorageImage(_normalImage.get());
    varianceMat->addStorageImage(_depthImage.get());
    // output
    varianceMat->addStorageImage(_gradientImage.get());
    varianceMat->addStorageImage(_varianceImage.get());
    varianceMat->addStorageImage(_varianceHistImage.get());
  }
  _varianceModel = std::make_unique<ComputeModel>(&_logger, std::move(varianceMat));

  _aTrousModels.clear();
  for (int i = 0; i < kATrousSize; i++) {
    auto aTrousMat =
        std::make_unique<ComputeMaterial>(kPathToResourceFolder + "/shaders/generated/aTrous.spv");
    {
      aTrousMat->addUniformBufferBundle(_globalBufferBundle.get());
      aTrousMat->addUniformBufferBundle(_blurFilterBufferBundles[i].get());
      // readonly input
      aTrousMat->addStorageImage(_vec2BlueNoise.get());
      aTrousMat->addStorageImage(_weightedCosineBlueNoise.get());
      // input
      aTrousMat->addStorageImage(_aTrousInputImage.get());
      aTrousMat->addStorageImage(_normalImage.get());
      aTrousMat->addStorageImage(_depthImage.get());
      aTrousMat->addStorageImage(_gradientImage.get());
      aTrousMat->addStorageImage(_varianceImage.get());
      // output
      aTrousMat->addStorageImage(_lastFrameAccumImage.get());
      aTrousMat->addStorageImage(_aTrousOutputImage.get());
    }
    _aTrousModels.emplace_back(std::make_unique<ComputeModel>(&_logger, std::move(aTrousMat)));
  }

  auto postProcessingMat = std::make_unique<ComputeMaterial>(
      kPathToResourceFolder + "/shaders/generated/postProcessing.spv");
  {
    postProcessingMat->addUniformBufferBundle(_globalBufferBundle.get());
    postProcessingMat->addUniformBufferBundle(_postProcessingBufferBundle.get());
    // input
    postProcessingMat->addStorageImage(_aTrousOutputImage.get());
    postProcessingMat->addStorageImage(_varianceImage.get());
    postProcessingMat->addStorageImage(_rawImage.get());
    postProcessingMat->addStorageImage(_stratumOffsetImage.get());
    postProcessingMat->addStorageImage(_visibilityImage.get());
    postProcessingMat->addStorageImage(_temporalGradientNormalizationImagePong.get());
    postProcessingMat->addStorageImage(_seedImage.get());
    // output
    postProcessingMat->addStorageImage(_targetImage.get());
  }
  _postProcessingModel = std::make_unique<ComputeModel>(&_logger, std::move(postProcessingMat));
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

  _globalBufferBundle->getBuffer(currentImageIndex)->fillData(&globalUbo);

  auto thisMvpe =
      _camera->getProjectionMatrix(static_cast<float>(_appContext->getSwapchainExtentWidth()) /
                                   static_cast<float>(_appContext->getSwapchainExtentHeight())) *
      _camera->getViewMatrix();
  GradientProjectionUniformBufferObject gpUbo = {!_useGradientProjection, thisMvpe};
  _gradientProjectionBufferBundle->getBuffer(currentImageIndex)->fillData(&gpUbo);

  RtxUniformBufferObject rtxUbo = {

      static_cast<uint32_t>(_rtScene->triangles.size()),
      static_cast<uint32_t>(_rtScene->lights.size()),
      _movingLightSource,
      _outputType,
      _offsetX,
      _offsetY};

  _rtxBufferBundle->getBuffer(currentImageIndex)->fillData(&rtxUbo);

  for (int i = 0; i < 6; i++) {
    StratumFilterUniformBufferObject sfUbo = {i, !_useStratumFiltering};
    _stratumFilterBufferBundle[i]->getBuffer(currentImageIndex)->fillData(&sfUbo);
  }

  TemporalFilterUniformBufferObject tfUbo = {!_useTemporalBlend, _useNormalTest, _normalThreshold,
                                             _blendingAlpha, lastMvpe};
  _temperalFilterBufferBundle->getBuffer(currentImageIndex)->fillData(&tfUbo);
  lastMvpe = thisMvpe;

  VarianceUniformBufferObject varianceUbo = {!_useVarianceEstimation, _skipStoppingFunctions,
                                             _useTemporalVariance,    _varianceKernelSize,
                                             _variancePhiGaussian,    _variancePhiDepth};
  _varianceBufferBundle->getBuffer(currentImageIndex)->fillData(&varianceUbo);

  for (int j = 0; j < kATrousSize; j++) {
    // update ubo for the sampleDistance
    BlurFilterUniformBufferObject bfUbo = {!_useATrous,
                                           j,
                                           _iCap,
                                           _useVarianceGuidedFiltering,
                                           _useGradientInDepth,
                                           _phiLuminance,
                                           _phiDepth,
                                           _phiNormal,
                                           _ignoreLuminanceAtFirstIteration,
                                           _changingLuminancePhi,
                                           _useJittering};
    _blurFilterBufferBundles[j]->getBuffer(currentImageIndex)->fillData(&bfUbo);
  }

  PostProcessingUniformBufferObject postProcessingUbo = {_displayType};
  _postProcessingBufferBundle->getBuffer(currentImageIndex)->fillData(&postProcessingUbo);

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

  for (size_t i = 0; i < _commandBuffers.size(); i++) {
    VkCommandBuffer &currentCommandBuffer = _commandBuffers[i];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(currentCommandBuffer, &beginInfo);
    assert(result == VK_SUCCESS && "vkBeginCommandBuffer failed");

    _targetImage->clearImage(currentCommandBuffer);
    _aTrousInputImage->clearImage(currentCommandBuffer);
    _aTrousOutputImage->clearImage(currentCommandBuffer);
    _stratumOffsetImage->clearImage(currentCommandBuffer);
    _perStratumLockingImage->clearImage(currentCommandBuffer);
    _visibilityImage->clearImage(currentCommandBuffer);
    _seedVisibilityImage->clearImage(currentCommandBuffer);
    _temporalGradientNormalizationImagePing->clearImage(currentCommandBuffer);
    _temporalGradientNormalizationImagePong->clearImage(currentCommandBuffer);

    _gradientProjectionModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                             ceil(_targetImage->getWidth() / 24.F),
                                             ceil(_targetImage->getHeight() / 24.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
                         nullptr);

    _rtxModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                              ceil(_targetImage->getWidth() / 32.F),
                              ceil(_targetImage->getHeight() / 32.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
                         nullptr);

    _gradientModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                   ceil(_targetImage->getWidth() / 32.F),
                                   ceil(_targetImage->getHeight() / 32.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
                         nullptr);

    for (int j = 0; j < 6; j++) {
      _stratumFilterModels[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                              ceil(_targetImage->getWidth() / 32.F),
                                              ceil(_targetImage->getHeight() / 32.F), 1);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
                           nullptr);
    }

    _temporalFilterModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                         ceil(_targetImage->getWidth() / 3.F / 32.F),
                                         ceil(_targetImage->getHeight() / 3.F / 32.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
                         nullptr);

    _varianceModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                   ceil(_targetImage->getWidth() / 32.F),
                                   ceil(_targetImage->getHeight() / 32.F), 1);

    vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
                         nullptr);

    /////////////////////////////////////////////

    for (int j = 0; j < kATrousSize; j++) {
      // dispatch filter shader
      _aTrousModels[j]->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                       ceil(_targetImage->getWidth() / 32.F),
                                       ceil(_targetImage->getHeight() / 32.F), 1);
      vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
                           nullptr);

      // copy aTrousImage2 to aTrousImage1 (excluding the last transfer)
      if (j != kATrousSize - 1) {
        _aTrousForwardingPair->forwardCopying(currentCommandBuffer);
      }
    }

    /////////////////////////////////////////////

    _postProcessingModel->computeCommand(currentCommandBuffer, static_cast<uint32_t>(i),
                                         ceil(_targetImage->getWidth() / 32.F),
                                         ceil(_targetImage->getHeight() / 32.F), 1);

    _targetForwardingPairs[i]->forwardCopying(currentCommandBuffer);

    // copy to history images
    _depthForwardingPair->forwardCopying(currentCommandBuffer);
    _normalForwardingPair->forwardCopying(currentCommandBuffer);
    _gradientForwardingPair->forwardCopying(currentCommandBuffer);
    _varianceHistForwardingPair->forwardCopying(currentCommandBuffer);
    _meshHashForwardingPair->forwardCopying(currentCommandBuffer);

    // Bind graphics pipeline and dispatch draw command.
    // postProcessScene->writeRenderCommand(currentCommandBuffer, i);

    result = vkEndCommandBuffer(currentCommandBuffer);
    assert(result == VK_SUCCESS && "vkEndCommandBuffer failed");
  }
}

void Application::_cleanupGuiFrameBuffers() {
  for (auto &guiFrameBuffer : _guiFrameBuffers) {
    vkDestroyFramebuffer(_appContext->getDevice(), guiFrameBuffer, nullptr);
  }
}

// not needed now because of smart pointers
void Application::_cleanupComputeModels() {}

// these commandbuffers are initially recorded with the models
void Application::_cleanupRenderCommandBuffers() {

  for (auto &commandBuffer : _commandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }
}
void Application::_cleanupSwapchainDimensionRelatedResources() {
  _cleanupComputeModels();
  _cleanupRenderCommandBuffers();
  _cleanupGuiFrameBuffers();
}

void Application::_createSwapchainDimensionRelatedResources() {
  _createImagesAndForwardingPairs();
  _createComputeModels();
  _createRenderCommandBuffers();
  _createGuiFramebuffers();
}

void Application::_vkCreateSemaphoresAndFences() {
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

void Application::_createGuiFramebuffers() {
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
  std::array<VkCommandBuffer, 2> submitCommandBuffers = {_commandBuffers[imageIndex],
                                                         _guiCommandBuffers[imageIndex]};

  VkSubmitInfo submitInfo{};
  {
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

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
  }

  result = vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo,
                         _framesInFlightFences[currentFrame]);
  assert(result == VK_SUCCESS && "vkQueueSubmit failed");

  VkPresentInfoKHR presentInfo{};
  {
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &_renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &_appContext->getSwapchain();
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;
  }

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
  _createBufferBundles();
  _createImagesAndForwardingPairs();
  _createComputeModels();

  // create render command buffers
  _createRenderCommandBuffers();

  // create GUI command buffers
  _createGuiCommandBuffers();

  // create synchronization objects
  _vkCreateSemaphoresAndFences();

  // create GUI render pass
  _createGuiRenderPass();

  // create GUI framebuffers
  _createGuiFramebuffers();

  // create GUI descriptor pool
  _createGuiDescripterPool();

  // initialize GUI
  _initGui();

  // set mouse callback function to be called whenever the cursor position
  // changes
  // glfwSetCursorPosCallback(mWindow->getGlWindow(), mouseCallback);

  _window->addMouseCallback([](float mouseDeltaX, float mouseDeltaY) {
    _camera->processMouseMovement(mouseDeltaX, mouseDeltaY);
  });
}