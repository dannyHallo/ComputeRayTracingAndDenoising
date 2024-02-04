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

// static int constexpr kStratumFilterSize   = 6;
// static int constexpr kATrousSize          = 5;
static uint32_t constexpr kBeamResolution = 8;

SvoTracer::SvoTracer(VulkanApplicationContext *appContext, Logger *logger, size_t framesInFlight,
                     Camera *camera)
    : _appContext(appContext), _logger(logger), _camera(camera), _framesInFlight(framesInFlight) {}

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
  _createBuffersAndBufferBundles();
  _initBufferData();

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
  _createResourseImages();
  _createSwapchainRelatedImages();
}

void SvoTracer::_createResourseImages() { _createBlueNoiseImages(); }

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

  // w = 16 -> 3, w = 17 -> 4
  _beamDepthImage = std::make_unique<Image>(std::ceil(static_cast<float>(w) / kBeamResolution) + 1,
                                            std::ceil(static_cast<float>(h) / kBeamResolution) + 1,
                                            VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _rawImage = std::make_unique<Image>(w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT);

  _positionImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _octreeVisualizationImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _normalImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _lastNormalImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _voxHashImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _lastVoxHashImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _accumedImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _lastAccumedImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _varianceHistImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,

                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _lastVarianceHistImage =
      std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _renderTargetImage = std::make_unique<Image>(
      w, h, VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  // _seedImage =
  //     std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_USAGE_STORAGE_BIT);

  // _aTrousInputImage =
  //     std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
  //                             VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _aTrousOutputImage =
  //     std::make_unique<Image>(w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
  //                             VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
  //                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _depthImage = std::make_unique<Image>(
  //     w, h, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  // _depthImagePrev = std::make_unique<Image>(
  //     w, h, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _gradientImage = std::make_unique<Image>(
  //     w, h, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT |
  //     VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  // _gradientImagePrev = std::make_unique<Image>(
  //     w, h, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT |
  //     VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _varianceImage = std::make_unique<Image>(w, h, VK_FORMAT_R32_SFLOAT,
  // VK_IMAGE_USAGE_STORAGE_BIT);
}

void SvoTracer::_createStratumSizedImages() {
  // auto w = _appContext->getSwapchainExtentWidth();
  // auto h = _appContext->getSwapchainExtentHeight();

  // constexpr float kStratumResolutionScale = 1.0F / 3.0F;

  // uint32_t perStratumImageWidth  = ceil(static_cast<float>(w) * kStratumResolutionScale);
  // uint32_t perStratumImageHeight = ceil(static_cast<float>(h) * kStratumResolutionScale);

  // _stratumOffsetImage =
  //     std::make_unique<Image>(perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32_UINT,
  //                             VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _perStratumLockingImage =
  //     std::make_unique<Image>(perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32_UINT,

  //                             VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _visibilityImage = std::make_unique<Image>(
  //     perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
  //     VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _seedVisibilityImage = std::make_unique<Image>(
  //     perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_UINT,
  //     VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _temporalGradientNormalizationImagePing = std::make_unique<Image>(
  //     perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
  //     VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // _temporalGradientNormalizationImagePong = std::make_unique<Image>(
  //     perStratumImageWidth, perStratumImageHeight, VK_FORMAT_R32G32B32A32_SFLOAT,
  //     VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}

void SvoTracer::_createImageForwardingPairs() {

  _normalForwardingPair = std::make_unique<ImageForwardingPair>(
      _normalImage->getVkImage(), _lastNormalImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _voxHashForwardingPair = std::make_unique<ImageForwardingPair>(
      _voxHashImage->getVkImage(), _lastVoxHashImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _accumedForwardingPair = std::make_unique<ImageForwardingPair>(
      _accumedImage->getVkImage(), _lastAccumedImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  // _aTrousForwardingPair = std::make_unique<ImageForwardingPair>(
  //     _aTrousOutputImage->getVkImage(), _aTrousInputImage->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
  //     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  // _depthForwardingPair = std::make_unique<ImageForwardingPair>(
  //     _depthImage->getVkImage(), _depthImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
  //     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  // _gradientForwardingPair = std::make_unique<ImageForwardingPair>(
  //     _gradientImage->getVkImage(), _gradientImagePrev->getVkImage(), VK_IMAGE_LAYOUT_GENERAL,
  //     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

  _varianceHistForwardingPair = std::make_unique<ImageForwardingPair>(
      _varianceHistImage->getVkImage(), _lastVarianceHistImage->getVkImage(),
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  // creating forwarding pairs to copy the image result each frame to a specific swapchain
  _targetForwardingPairs.clear();
  for (int i = 0; i < _appContext->getSwapchainSize(); i++) {
    _targetForwardingPairs.emplace_back(std::make_unique<ImageForwardingPair>(
        _renderTargetImage->getVkImage(), _appContext->getSwapchainImages()[i],
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  }
}

// these buffers are modified by the CPU side every frame, and we have multiple frames in flight,
// so we need to create multiple copies of them, they are fairly small though
void SvoTracer::_createBuffersAndBufferBundles() {
  // buffers
  _sceneDataBuffer = std::make_unique<Buffer>(
      sizeof(G_SceneData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryAccessingStyle::kCpuToGpuOnce);

  // buffer bundles
  _renderInfoUniformBuffers = std::make_unique<BufferBundle>(
      _framesInFlight, sizeof(G_RenderInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      MemoryAccessingStyle::kCpuToGpuEveryFrame);

  _twickableParametersUniformBuffers = std::make_unique<BufferBundle>(
      _framesInFlight, sizeof(G_TwickableParameters), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      MemoryAccessingStyle::kCpuToGpuEveryFrame);
}

void SvoTracer::_initBufferData() {
  G_SceneData sceneData = {kBeamResolution, _svoBuilder->getVoxelLevelCount()};
  _sceneDataBuffer->fillData(&sceneData);
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

  // create the general memory barrier
  VkMemoryBarrier memoryBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  memoryBarrier.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  for (size_t imageIndex = 0; imageIndex < _commandBuffers.size(); imageIndex++) {
    auto &cmdBuffer = _commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    _renderTargetImage->clearImage(cmdBuffer);
    // _aTrousInputImage->clearImage(cmdBuffer);
    // _aTrousOutputImage->clearImage(cmdBuffer);
    // _stratumOffsetImage->clearImage(cmdBuffer);
    // _perStratumLockingImage->clearImage(cmdBuffer);
    // _visibilityImage->clearImage(cmdBuffer);
    // _seedVisibilityImage->clearImage(cmdBuffer);
    // _temporalGradientNormalizationImagePing->clearImage(cmdBuffer);
    // _temporalGradientNormalizationImagePong->clearImage(cmdBuffer);

    uint32_t w = _appContext->getSwapchainExtentWidth();
    uint32_t h = _appContext->getSwapchainExtentHeight();

    // _PipelinesHolder->getGradientProjectionModel()->computeCommand(cmdBuffer, imageIndex, w, h,
    // 1);

    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0,
    //                      nullptr);

    _svoCourseBeamPipeline->recordCommand(
        cmdBuffer, 0, static_cast<uint32_t>(std::ceil(static_cast<float>(w) / kBeamResolution)) + 1,
        static_cast<uint32_t>(std::ceil(static_cast<float>(h) / kBeamResolution)) + 1, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _svoTracingPipeline->recordCommand(cmdBuffer, 0, w, h, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _temporalFilterPipeline->recordCommand(cmdBuffer, 0, w, h, 1);

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

    // copy to history images
    _normalForwardingPair->forwardCopy(cmdBuffer);
    _voxHashForwardingPair->forwardCopy(cmdBuffer);
    _accumedForwardingPair->forwardCopy(cmdBuffer);
    _varianceHistForwardingPair->forwardCopy(cmdBuffer);
    // _depthForwardingPair->forwardCopy(cmdBuffer);
    // _gradientForwardingPair->forwardCopy(cmdBuffer);
    // _varianceHistForwardingPair->forwardCopy(cmdBuffer);

    _targetForwardingPairs[imageIndex]->forwardCopy(cmdBuffer);

    vkEndCommandBuffer(cmdBuffer);
  }
}

void SvoTracer::updateUboData(size_t currentFrame) {
  static uint32_t currentSample = 0;
  static glm::mat4 lastMvpe{1.0F}; // identity matrix

  auto currentTime = static_cast<float>(glfwGetTime());

  auto thisMvpe =
      _camera->getProjectionMatrix(static_cast<float>(_appContext->getSwapchainExtentWidth()) /
                                   static_cast<float>(_appContext->getSwapchainExtentHeight())) *
      _camera->getViewMatrix();

  G_RenderInfo renderInfoData = {
      _camera->getPosition(),
      _camera->getFront(),
      _camera->getUp(),
      _camera->getRight(),
      thisMvpe,
      lastMvpe,
      _appContext->getSwapchainExtentWidth(),
      _appContext->getSwapchainExtentHeight(),
      _camera->getVFov(),
      currentSample,
      currentTime,
  };
  _renderInfoUniformBuffers->getBuffer(currentFrame)->fillData(&renderInfoData);

  lastMvpe = thisMvpe;

  G_TwickableParameters gpUbo{};
  gpUbo.magicButton      = static_cast<uint32_t>(_uboData.magicButton);
  gpUbo.visualizeOctree  = static_cast<uint32_t>(_uboData.visualizeOctree);
  gpUbo.beamOptimization = static_cast<uint32_t>(_uboData.beamOptimization);
  gpUbo.temporalAlpha    = _uboData.temporalAlpha;

  _twickableParametersUniformBuffers->getBuffer(currentFrame)->fillData(&gpUbo);

  // _gradientProjectionBufferBundle->getBuffer(currentFrame)->fillData(&gpUbo);

  // for (int i = 0; i < kStratumFilterSize; i++) {
  //   StratumFilterUniformBufferObject sfUbo = {i,
  //   static_cast<int>(!_uboData._useStratumFiltering)};
  //   _stratumFilterBufferBundle[i]->getBuffer(currentFrame)->fillData(&sfUbo);
  // }

  // TemporalFilterUniformBufferObject tfUbo = {
  //     static_cast<int>(!_uboData._useTemporalBlend),
  //     static_cast<int>(_uboData._useNormalTest),
  //     _uboData._normalThreshold,
  //     _uboData._blendingAlpha,
  //     lastMvpe,
  // };
  // _temperalFilterBufferBundle->getBuffer(currentFrame)->fillData(&tfUbo);

  // VarianceUniformBufferObject varianceUbo = {
  //     static_cast<int>(!_uboData._useVarianceEstimation),
  //     static_cast<int>(_uboData._skipStoppingFunctions),
  //     static_cast<int>(_uboData._useTemporalVariance),
  //     _uboData._varianceKernelSize,
  //     _uboData._variancePhiGaussian,
  //     _uboData._variancePhiDepth,
  // };
  // _varianceBufferBundle->getBuffer(currentFrame)->fillData(&varianceUbo);

  // for (int i = 0; i < kATrousSize; i++) {
  //   // update ubo for the sampleDistance
  //   BlurFilterUniformBufferObject bfUbo = {
  //       static_cast<int>(!_uboData._useATrous),
  //       i,
  //       _uboData._iCap,
  //       static_cast<int>(_uboData._useVarianceGuidedFiltering),
  //       static_cast<int>(_uboData._useGradientInDepth),
  //       _uboData._phiLuminance,
  //       _uboData._phiDepth,
  //       _uboData._phiNormal,
  //       static_cast<int>(_uboData._ignoreLuminanceAtFirstIteration),
  //       static_cast<int>(_uboData._changingLuminancePhi),
  //       static_cast<int>(_uboData._useJittering),
  //   };
  //   _blurFilterBufferBundles[i]->getBuffer(currentFrame)->fillData(&bfUbo);
  // }

  // PostProcessingUniformBufferObject postProcessingUbo = {_uboData._displayType};
  // _postProcessingBufferBundle->getBuffer(currentFrame)->fillData(&postProcessingUbo);

  currentSample++;
}

void SvoTracer::_createDescriptorSetBundle() {
  _descriptorSetBundle = std::make_unique<DescriptorSetBundle>(_appContext, _framesInFlight,
                                                               VK_SHADER_STAGE_COMPUTE_BIT);

  _descriptorSetBundle->bindUniformBufferBundle(0, _renderInfoUniformBuffers.get());
  _descriptorSetBundle->bindUniformBufferBundle(1, _twickableParametersUniformBuffers.get());

  _descriptorSetBundle->bindStorageImage(2, _vec2BlueNoise.get());
  _descriptorSetBundle->bindStorageImage(3, _weightedCosineBlueNoise.get());

  _descriptorSetBundle->bindStorageImage(4, _beamDepthImage.get());
  _descriptorSetBundle->bindStorageImage(5, _rawImage.get());
  _descriptorSetBundle->bindStorageImage(6, _positionImage.get());
  _descriptorSetBundle->bindStorageImage(7, _octreeVisualizationImage.get());
  _descriptorSetBundle->bindStorageImage(8, _normalImage.get());
  _descriptorSetBundle->bindStorageImage(9, _lastNormalImage.get());
  _descriptorSetBundle->bindStorageImage(10, _voxHashImage.get());
  _descriptorSetBundle->bindStorageImage(11, _lastVoxHashImage.get());
  _descriptorSetBundle->bindStorageImage(12, _accumedImage.get());
  _descriptorSetBundle->bindStorageImage(13, _lastAccumedImage.get());
  _descriptorSetBundle->bindStorageImage(14, _varianceHistImage.get());
  _descriptorSetBundle->bindStorageImage(15, _lastVarianceHistImage.get());
  _descriptorSetBundle->bindStorageImage(16, _renderTargetImage.get());

  _descriptorSetBundle->bindStorageBuffer(17, _sceneDataBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(18, _svoBuilder->getOctreeBuffer());
  _descriptorSetBundle->bindStorageBuffer(19, _svoBuilder->getPaletteBuffer());

  _descriptorSetBundle->create();
}

void SvoTracer::_createPipelines() {
  {
    std::vector<uint32_t> shaderCode{
#include "svoCoarseBeam.spv"
    };
    _svoCourseBeamPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get());
    _svoCourseBeamPipeline->init();
  }
  {
    std::vector<uint32_t> shaderCode{
#include "svoTracing.spv"
    };
    _svoTracingPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get());
    _svoTracingPipeline->init();
  }
  {
    std::vector<uint32_t> shaderCode{
#include "temporalFilter.spv"
    };
    _temporalFilterPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get());
    _temporalFilterPipeline->init();
  }
  {
    std::vector<uint32_t> shaderCode{
#include "postProcessing.spv"
    };
    _postProcessingPipeline =
        std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                          WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get());
    _postProcessingPipeline->init();
  }
}
