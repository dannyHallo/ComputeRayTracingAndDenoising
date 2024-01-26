#include "SvoTracer.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "memory/Buffer.hpp"
#include "memory/BufferBundle.hpp"
#include "memory/Image.hpp"
#include "pipelines/ComputePipeline.hpp"
#include "pipelines/DescriptorSetBundle.hpp"
#include "svo-builder/SvoBuilder.hpp"
#include "utils/camera/Camera.hpp"
#include "utils/config/RootDir.h"

static int constexpr kStratumFilterSize = 6;
static int constexpr kATrousSize        = 5;

SvoTracer::SvoTracer(VulkanApplicationContext *appContext, Logger *logger, size_t framesInFlight,
                     Camera *camera)
    : _appContext(appContext), _logger(logger), _camera(camera), _framesInFlight(framesInFlight),
      _uboData(framesInFlight) {}

SvoTracer::~SvoTracer() {
  for (auto &commandBuffer : _commandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }
}

void SvoTracer::init(SvoBuilder *svoBuilder) {
  _svoBuilder = svoBuilder;

  // images
  _createImages();
  _createImageForwardingPairs();

  // buffers
  _createBufferBundles();

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();

  // create command buffers
  _recordCommandBuffers();
}

void SvoTracer::onSwapchainResize() {
  // images
  _createSwapchainRelatedImages();
  _createImageForwardingPairs();

  // pipelines
  _createDescriptorSetBundle();
  _createPipelines();

  _recordCommandBuffers();
}

void SvoTracer::_createImages() {
  _createOneTimeImages();
  _createSwapchainRelatedImages();
}

void SvoTracer::_createOneTimeImages() { _createBlueNoiseImages(); }

void SvoTracer::_createSwapchainRelatedImages() {
  _createFullSizedImages();
  _createStratumSizedImages();
}

void SvoTracer::_createBlueNoiseImages() {
  std::vector<std::string> filenames{};
  constexpr int kVec2BlueNoiseSize           = 64;
  constexpr int kWeightedCosineBlueNoiseSize = 64;

  filenames.reserve(kVec2BlueNoiseSize);
  for (int i = 0; i < kVec2BlueNoiseSize; i++) {
    filenames.emplace_back(kPathToResourceFolder +
                           "/textures/stbn/vec2_2d_1d/"
                           "stbn_vec2_2Dx1D_128x128x64_" +
                           std::to_string(i) + ".png");
  }
  _vec2BlueNoise = std::make_unique<Image>(filenames, VK_IMAGE_USAGE_STORAGE_BIT |
                                                          VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  filenames.clear();
  filenames.reserve(kWeightedCosineBlueNoiseSize);
  for (int i = 0; i < kWeightedCosineBlueNoiseSize; i++) {
    filenames.emplace_back(kPathToResourceFolder +
                           "/textures/stbn/unitvec3_cosine_2d_1d/"
                           "stbn_unitvec3_cosine_2Dx1D_128x128x64_" +
                           std::to_string(i) + ".png");
  }
  _weightedCosineBlueNoise = std::make_unique<Image>(
      filenames, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}

void SvoTracer::_createFullSizedImages() {
  auto w = _appContext->getSwapchainExtentWidth();
  auto h = _appContext->getSwapchainExtentHeight();

  _targetImage = std::make_unique<Image>(w, h, VK_FORMAT_R8G8B8A8_UNORM,
                                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  _rawImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _seedImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_USAGE_STORAGE_BIT);

  _aTrousInputImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _aTrousOutputImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _positionImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _depthImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _depthImagePrev = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _normalImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _normalImagePrev =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _gradientImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _gradientImagePrev = std::make_unique<Image>(
      w, h, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _varianceHistImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _varianceHistImagePrev =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _meshHashImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _meshHashImagePrev = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _varianceImage = std::make_unique<Image>(w, h, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _lastFrameAccumImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);
}

void SvoTracer::_createStratumSizedImages() {
  auto w = _appContext->getSwapchainExtentWidth();
  auto h = _appContext->getSwapchainExtentHeight();

  constexpr float kStratumResolutionScale = 1.0F / 3.0F;

  uint32_t perStratumImageWidth  = ceil(static_cast<float>(w) * kStratumResolutionScale);
  uint32_t perStratumImageHeight = ceil(static_cast<float>(h) * kStratumResolutionScale);

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

void SvoTracer::_createImageForwardingPairs() {
  // creating forwarding pairs to copy the image result each frame to a specific swapchain
  _targetForwardingPairs.clear();
  for (int i = 0; i < _appContext->getSwapchainSize(); i++) {
    _targetForwardingPairs.emplace_back(std::make_unique<ImageForwardingPair>(
        _targetImage->getVkImage(), _appContext->getSwapchainImages()[i], VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  }

  _aTrousForwardingPair = std::make_unique<ImageForwardingPair>(
      _aTrousOutputImage->getVkImage(), _aTrousInputImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _depthForwardingPair = std::make_unique<ImageForwardingPair>(
      _depthImage->getVkImage(), _depthImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _normalForwardingPair = std::make_unique<ImageForwardingPair>(
      _normalImage->getVkImage(), _normalImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _gradientForwardingPair = std::make_unique<ImageForwardingPair>(
      _gradientImage->getVkImage(), _gradientImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _varianceHistForwardingPair = std::make_unique<ImageForwardingPair>(
      _varianceHistImage->getVkImage(), _varianceHistImagePrev->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  _meshHashForwardingPair = std::make_unique<ImageForwardingPair>(
      _meshHashImage->getVkImage(), _meshHashImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
}

// these buffers are modified by the CPU side every frame, and we have multiple frames in flight,
// so we need to create multiple copies of them, they are fairly small though
void SvoTracer::_createBufferBundles() {
  _globalBufferBundle = std::make_unique<BufferBundle>(
      _framesInFlight, sizeof(GlobalUniformBufferObject), BufferType::kUniform);

  _gradientProjectionBufferBundle = std::make_unique<BufferBundle>(
      _framesInFlight, sizeof(GradientProjectionUniformBufferObject), BufferType::kUniform);

  for (int i = 0; i < kStratumFilterSize; i++) {
    auto stratumFilterBufferBundle = std::make_unique<BufferBundle>(
        _framesInFlight, sizeof(StratumFilterUniformBufferObject), BufferType::kUniform);
    _stratumFilterBufferBundle.emplace_back(std::move(stratumFilterBufferBundle));
  }

  _temperalFilterBufferBundle = std::make_unique<BufferBundle>(
      _framesInFlight, sizeof(TemporalFilterUniformBufferObject), BufferType::kUniform);

  _varianceBufferBundle = std::make_unique<BufferBundle>(
      _framesInFlight, sizeof(VarianceUniformBufferObject), BufferType::kUniform);

  for (int i = 0; i < kATrousSize; i++) {
    auto blurFilterBufferBundle = std::make_unique<BufferBundle>(
        _framesInFlight, sizeof(BlurFilterUniformBufferObject), BufferType::kUniform);
    _blurFilterBufferBundles.emplace_back(std::move(blurFilterBufferBundle));
  }

  _postProcessingBufferBundle = std::make_unique<BufferBundle>(
      _framesInFlight, sizeof(PostProcessingUniformBufferObject), BufferType::kUniform);
}

void SvoTracer::_recordCommandBuffers() {
  for (auto &commandBuffer : _commandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }
  _commandBuffers.clear();

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
    auto &cmdBuffer = _commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    assert(result == VK_SUCCESS && "vkBeginCommandBuffer failed");

    _targetImage->clearImage(cmdBuffer);
    _aTrousInputImage->clearImage(cmdBuffer);
    _aTrousOutputImage->clearImage(cmdBuffer);
    _stratumOffsetImage->clearImage(cmdBuffer);
    _perStratumLockingImage->clearImage(cmdBuffer);
    _visibilityImage->clearImage(cmdBuffer);
    _seedVisibilityImage->clearImage(cmdBuffer);
    _temporalGradientNormalizationImagePing->clearImage(cmdBuffer);
    _temporalGradientNormalizationImagePong->clearImage(cmdBuffer);

    uint32_t w = _appContext->getSwapchainExtentWidth();
    uint32_t h = _appContext->getSwapchainExtentHeight();

    // _PipelinesHolder->getGradientProjectionModel()->computeCommand(cmdBuffer, imageIndex, w, h,
    // 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    _svoPipeline->recordCommand(cmdBuffer, 0, w, h, 1);

    VkMemoryBarrier memoryBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    memoryBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    // _PipelinesHolder->getGradientModel()->computeCommand(cmdBuffer, imageIndex, w, h, 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    // for (int j = 0; j < 6; j++) {
    //   _PipelinesHolder->getStratumFilterModel(j)->computeCommand(cmdBuffer, imageIndex, w, h, 1);

    //   vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                        nullptr);
    // }

    // _PipelinesHolder->getTemporalFilterModel()->computeCommand(cmdBuffer, imageIndex, w, h, 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    // _PipelinesHolder->getVarianceModel()->computeCommand(cmdBuffer, imageIndex, w, h, 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    // for (int j = 0; j < kATrousSize; j++) {
    //   // dispatch filter shader
    //   _PipelinesHolder->getATrousModel(j)->computeCommand(cmdBuffer, imageIndex, w, h, 1);
    //   vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                        nullptr);

    //   // copy aTrousImage2 to aTrousImage1 (excluding the last transfer)
    //   if (j != kATrousSize - 1) {
    //     _SvoTracer->getATrousForwardingPair()->forwardCopy(cmdBuffer);
    //   }
    // }

    _postProcessingPipeline->recordCommand(cmdBuffer, 0, w, h, 1);

    _targetForwardingPairs[imageIndex]->forwardCopy(cmdBuffer);

    // copy to history images
    _depthForwardingPair->forwardCopy(cmdBuffer);
    _normalForwardingPair->forwardCopy(cmdBuffer);
    _gradientForwardingPair->forwardCopy(cmdBuffer);
    _varianceHistForwardingPair->forwardCopy(cmdBuffer);
    _meshHashForwardingPair->forwardCopy(cmdBuffer);

    result = vkEndCommandBuffer(cmdBuffer);
    assert(result == VK_SUCCESS && "vkEndCommandBuffer failed");
  }
}

void SvoTracer::updateUboData(size_t currentFrame) {
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
  _globalBufferBundle->getBuffer(currentFrame)->fillData(&globalUbo);

  auto thisMvpe =
      _camera->getProjectionMatrix(static_cast<float>(_appContext->getSwapchainExtentWidth()) /
                                   static_cast<float>(_appContext->getSwapchainExtentHeight())) *
      _camera->getViewMatrix();
  GradientProjectionUniformBufferObject gpUbo = {
      static_cast<int>(!_uboData._useGradientProjection),
      thisMvpe,
  };
  _gradientProjectionBufferBundle->getBuffer(currentFrame)->fillData(&gpUbo);

  for (int i = 0; i < kStratumFilterSize; i++) {
    StratumFilterUniformBufferObject sfUbo = {i, static_cast<int>(!_uboData._useStratumFiltering)};
    _stratumFilterBufferBundle[i]->getBuffer(currentFrame)->fillData(&sfUbo);
  }

  TemporalFilterUniformBufferObject tfUbo = {
      static_cast<int>(!_uboData._useTemporalBlend),
      static_cast<int>(_uboData._useNormalTest),
      _uboData._normalThreshold,
      _uboData._blendingAlpha,
      lastMvpe,
  };
  _temperalFilterBufferBundle->getBuffer(currentFrame)->fillData(&tfUbo);
  lastMvpe = thisMvpe;

  VarianceUniformBufferObject varianceUbo = {
      static_cast<int>(!_uboData._useVarianceEstimation),
      static_cast<int>(_uboData._skipStoppingFunctions),
      static_cast<int>(_uboData._useTemporalVariance),
      _uboData._varianceKernelSize,
      _uboData._variancePhiGaussian,
      _uboData._variancePhiDepth,
  };
  _varianceBufferBundle->getBuffer(currentFrame)->fillData(&varianceUbo);

  for (int i = 0; i < kATrousSize; i++) {
    // update ubo for the sampleDistance
    BlurFilterUniformBufferObject bfUbo = {
        static_cast<int>(!_uboData._useATrous),
        i,
        _uboData._iCap,
        static_cast<int>(_uboData._useVarianceGuidedFiltering),
        static_cast<int>(_uboData._useGradientInDepth),
        _uboData._phiLuminance,
        _uboData._phiDepth,
        _uboData._phiNormal,
        static_cast<int>(_uboData._ignoreLuminanceAtFirstIteration),
        static_cast<int>(_uboData._changingLuminancePhi),
        static_cast<int>(_uboData._useJittering),
    };
    _blurFilterBufferBundles[i]->getBuffer(currentFrame)->fillData(&bfUbo);
  }

  PostProcessingUniformBufferObject postProcessingUbo = {_uboData._displayType};
  _postProcessingBufferBundle->getBuffer(currentFrame)->fillData(&postProcessingUbo);

  currentSample++;
}

void SvoTracer::_createDescriptorSetBundle() {
  _descriptorSetBundle = std::make_unique<DescriptorSetBundle>(
      _appContext, _logger, _framesInFlight, VK_SHADER_STAGE_COMPUTE_BIT);

  _descriptorSetBundle->bindUniformBufferBundle(0, _globalBufferBundle.get());
  _descriptorSetBundle->bindUniformBufferBundle(1, _postProcessingBufferBundle.get());

  _descriptorSetBundle->bindStorageImage(2, _rawImage.get());
  _descriptorSetBundle->bindStorageImage(3, _targetImage.get()); // NOLINT

  _descriptorSetBundle->bindStorageBuffer(4, _svoBuilder->getVoxelBuffer());
  _descriptorSetBundle->bindStorageBuffer(5, _svoBuilder->getPaletteBuffer()); // NOLINT

  _descriptorSetBundle->create();
}

void SvoTracer::_createPipelines() {
  {
    std::vector<uint32_t> shaderCode{
#include "svoTracing.spv"
    };
    _svoPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get());
    _svoPipeline->create();
  }
  {
    std::vector<uint32_t> shaderCode{
#include "postProcessing.spv"
    };
    _postProcessingPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get());
    _postProcessingPipeline->create();
  }
}
