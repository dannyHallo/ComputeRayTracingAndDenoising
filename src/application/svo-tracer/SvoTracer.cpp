#include "SvoTracer.hpp"

#include "../svo-builder/SvoBuilder.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "camera/Camera.hpp"
#include "camera/ShadowMapCamera.hpp"
#include "file-watcher/ShaderChangeListener.hpp"
#include "utils/config/RootDir.h"
#include "utils/io/ShaderFileReader.hpp"
#include "utils/toml-config/TomlConfigReader.hpp"
#include "vulkan-wrapper/descriptor-set/DescriptorSetBundle.hpp"
#include "vulkan-wrapper/memory/Buffer.hpp"
#include "vulkan-wrapper/memory/BufferBundle.hpp"
#include "vulkan-wrapper/memory/Image.hpp"
#include "vulkan-wrapper/pipeline/ComputePipeline.hpp"
#include "vulkan-wrapper/sampler/Sampler.hpp"

#include <string>

// should also be synchronized with the shader
constexpr uint32_t kTransmittanceLutWidth    = 256;
constexpr uint32_t kTransmittanceLutHeight   = 64;
constexpr uint32_t kMultiScatteringLutWidth  = 32;
constexpr uint32_t kMultiScatteringLutHeight = 32;
constexpr uint32_t kSkyViewLutWidth          = 200;
constexpr uint32_t kSkyViewLutHeight         = 200;

namespace {
float halton(int base, int index) {
  float f = 1.F;
  float r = 0.F;
  int i   = index;

  while (i > 0) {
    f = f / static_cast<float>(base);
    r = r + f * static_cast<float>(i % base);
    i = i / base;
  }
  return r;
};

std::string _makeShaderFullPath(std::string const &shaderName) {
  return kPathToResourceFolder + "shaders/svo-tracer/" + shaderName;
}

// translated from shader code
double constexpr kPi = 3.14159265358979323846;

// input: theta is the azimuthal angle
//        phi is the polar angle (from the y-axis)
glm::vec3 _getSphericalDir(float theta, float phi) {
  float sinPhi = sin(phi);
  return glm::vec3(sinPhi * sin(theta), cos(phi), sinPhi * cos(theta));
}

glm::vec3 _getDirOnUnitSphere(float alt, float azi) {
  return _getSphericalDir(azi, kPi / 2.F - alt);
}

glm::vec3 _getSunDir(float sunAltitude, float sunAzimuth) {
  return _getDirOnUnitSphere(glm::radians(sunAltitude), glm::radians(sunAzimuth));
}
}; // namespace

SvoTracer::SvoTracer(VulkanApplicationContext *appContext, Logger *logger, size_t framesInFlight,
                     Window *window, ShaderCompiler *shaderCompiler,
                     ShaderChangeListener *shaderChangeListener, TomlConfigReader *tomlConfigReader)
    : _appContext(appContext), _logger(logger), _window(window), _shaderCompiler(shaderCompiler),
      _shaderChangeListener(shaderChangeListener), _tomlConfigReader(tomlConfigReader),
      _tweakingData(tomlConfigReader), _framesInFlight(framesInFlight) {
  _camera          = std::make_unique<Camera>(_window, _tomlConfigReader);
  _shadowMapCamera = std::make_unique<ShadowMapCamera>(_tomlConfigReader);

  _loadConfig();
  _updateImageResolutions();
}

SvoTracer::~SvoTracer() {
  for (auto &commandBuffer : _tracingCommandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }
}

void SvoTracer::_loadConfig() {
  _aTrousSizeMax  = _tomlConfigReader->getConfig<uint32_t>("SvoTracer.aTrousSizeMax");
  _beamResolution = _tomlConfigReader->getConfig<uint32_t>("SvoTracer.beamResolution");
  _taaSamplingOffsetSize =
      _tomlConfigReader->getConfig<uint32_t>("SvoTracer.taaSamplingOffsetSize");
  _shadowMapResolution = _tomlConfigReader->getConfig<uint32_t>("SvoTracer.shadowMapResolution");
  _upscaleRatio        = _tomlConfigReader->getConfig<float>("SvoTracer.upscaleRatio");
}

void SvoTracer::processInput(double deltaTime) { _camera->processInput(deltaTime); }

void SvoTracer::_updateImageResolutions() {
  _highResWidth  = _appContext->getSwapchainExtentWidth();
  _highResHeight = _appContext->getSwapchainExtentHeight();

  _lowResWidth  = static_cast<uint32_t>(_highResWidth / _upscaleRatio);
  _lowResHeight = static_cast<uint32_t>(_highResHeight / _upscaleRatio);

  _logger->info("high res: {}x{}", _highResWidth, _highResHeight);
  _logger->info("low res: {}x{}", _lowResWidth, _lowResHeight);
}

void SvoTracer::init(SvoBuilder *svoBuilder) {
  _svoBuilder = svoBuilder;

  _createSamplers();

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
  _recordRenderingCommandBuffers();
  _recordDeliveryCommandBuffers();

  _createTaaSamplingOffsets();

  // attach camera's mouse handler to the window mouse callback
  _window->addCursorMoveCallback(
      [this](CursorMoveInfo const &mouseInfo) { _camera->handleMouseMovement(mouseInfo); });
}

void SvoTracer::onSwapchainResize() {
  _updateImageResolutions();

  // images
  _createSwapchainRelatedImages();
  _createImageForwardingPairs();

  // pipelines
  _createDescriptorSetBundle();
  _updatePipelinesDescriptorBundles();

  _recordRenderingCommandBuffers();
  _recordDeliveryCommandBuffers();
}

void SvoTracer::_createTaaSamplingOffsets() {
  _subpixOffsets.resize(_taaSamplingOffsetSize);
  for (int i = 0; i < _taaSamplingOffsetSize; i++) {
    _subpixOffsets[i] = {halton(2, i + 1) - 0.5F, halton(3, i + 1) - 0.5F};
    // _subpixOffsets[i] = {0, 0};
  }
}

void SvoTracer::update() { _recordRenderingCommandBuffers(); }

void SvoTracer::_createSamplers() {
  {
    auto settings         = Sampler::Settings{};
    settings.addressModeU = Sampler::AddressMode::kClampToEdge;
    settings.addressModeV = Sampler::AddressMode::kClampToEdge;
    settings.addressModeW = Sampler::AddressMode::kClampToEdge;
    _defaultSampler       = std::make_unique<Sampler>(settings);
  }

  {
    auto settings = Sampler::Settings{};
    // uv.x encodes the azimuth from -pi to pi, it needs to be closed
    settings.addressModeU = Sampler::AddressMode::kRepeat;
    settings.addressModeV = Sampler::AddressMode::kClampToEdge;
    settings.addressModeW = Sampler::AddressMode::kClampToEdge;
    _skyLutSampler        = std::make_unique<Sampler>(settings);
  }
}

void SvoTracer::_createImages() {
  _createBlueNoiseImages();
  _createSkyLutImages();
  _createShadowMapImage();
  _createSwapchainRelatedImages();
}

void SvoTracer::_createSwapchainRelatedImages() { _createFullSizedImages(); }

void SvoTracer::_createBlueNoiseImages() {
  auto _loadNoise = [this](std::unique_ptr<Image> &noiseImage, std::string const &&stbnPath) {
    _logger->info("loading blue noise images from {}", stbnPath);

    constexpr int kBlueNoiseArraySize = 64;
    std::vector<std::string> filenames{};
    filenames.reserve(kBlueNoiseArraySize);
    for (int i = 0; i < kBlueNoiseArraySize; i++) {
      filenames.emplace_back(kPathToResourceFolder + "/textures/stbn/" + stbnPath +
                             std::to_string(i) + ".png");
    }
    noiseImage = std::make_unique<Image>(filenames, VK_IMAGE_USAGE_STORAGE_BIT |
                                                        VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  };

  _loadNoise(_scalarBlueNoise, "scalar_2d_1d_1d/stbn_scalar_2Dx1Dx1D_128x128x64x1_");
  _loadNoise(_vec2BlueNoise, "vec2_2d_1d/stbn_vec2_2Dx1D_128x128x64_");
  _loadNoise(_vec3BlueNoise, "vec3_2d_1d/stbn_vec3_2Dx1D_128x128x64_");
  _loadNoise(_weightedCosineBlueNoise,
             "unitvec3_cosine_2d_1d/stbn_unitvec3_cosine_2Dx1D_128x128x64_");
}

void SvoTracer::_createSkyLutImages() {
  _transmittanceLutImage = std::make_unique<Image>(
      kTransmittanceLutWidth, kTransmittanceLutHeight, 1, VK_FORMAT_R16G16B16A16_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, _defaultSampler->getVkSampler());

  _multiScatteringLutImage = std::make_unique<Image>(
      kMultiScatteringLutWidth, kMultiScatteringLutHeight, 1, VK_FORMAT_R16G16B16A16_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, _defaultSampler->getVkSampler());

  _skyViewLutImage = std::make_unique<Image>(
      kSkyViewLutWidth, kSkyViewLutHeight, 1, VK_FORMAT_R16G16B16A16_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, _skyLutSampler->getVkSampler());
}

void SvoTracer::_createShadowMapImage() {
  _shadowMapImage = std::make_unique<Image>(
      _shadowMapResolution, _shadowMapResolution, 1, VK_FORMAT_R32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, _defaultSampler->getVkSampler());
}

// https://docs.vulkan.org/spec/latest/chapters/formats.html
void SvoTracer::_createFullSizedImages() {
  // the sky hdr view is very sensitive to gradient, so a high precision format is a must
  _backgroundImage = std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                                             VK_IMAGE_USAGE_STORAGE_BIT);

  // w = 16 -> 3, w = 17 -> 4
  _beamDepthImage = std::make_unique<Image>(
      std::ceil(static_cast<float>(_lowResWidth) / static_cast<float>(_beamResolution)) + 1,
      std::ceil(static_cast<float>(_lowResHeight) / static_cast<float>(_beamResolution)) + 1, 1,
      VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);

  _rawImage = std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                                      VK_IMAGE_USAGE_STORAGE_BIT);

  _godRayImage = std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                                         VK_IMAGE_USAGE_STORAGE_BIT);

  _depthImage = std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_SFLOAT,
                                        VK_IMAGE_USAGE_STORAGE_BIT);

  _octreeVisualizationImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_B10G11R11_UFLOAT_PACK32,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _hitImage = std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R8_UINT,
                                      VK_IMAGE_USAGE_STORAGE_BIT);

  _temporalHistLengthImage = std::make_unique<Image>(_lowResWidth, _lowResHeight, 1,
                                                     VK_FORMAT_R8_UINT, VK_IMAGE_USAGE_STORAGE_BIT);

  _motionImage = std::make_unique<Image>(_lowResWidth, _lowResHeight, 1,
                                         VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT);
  _normalImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _lastNormalImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _positionImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  _lastPositionImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32G32B32A32_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _voxHashImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _lastVoxHashImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // precision issues occurred when using VK_FORMAT_B10G11R11_UFLOAT_PACK32 to store hdr accumed
  // results, it can be observed when using a very low alpha blending value.
  // so either use VK_FORMAT_R32_UINT with custom RGBE packer / unpacker
  _accumedImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _lastAccumedImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  _godRayAccumedImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  _lastGodRayAccumedImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // same for taa images, use VK_FORMAT_R16G16B16A16_SFLOAT to enable accelerated sampling
  _taaImage =
      std::make_unique<Image>(_highResWidth, _highResHeight, 1, VK_FORMAT_R16G16B16A16_SFLOAT,
                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  _lastTaaImage = std::make_unique<Image>(
      _highResWidth, _highResHeight, 1, VK_FORMAT_R16G16B16A16_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      _defaultSampler->getVkSampler());

  _blittedImage = std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_R32_UINT,
                                          VK_IMAGE_USAGE_STORAGE_BIT);

  // both of the ping and pong can be dumped to the render target image and the lastAccumedImage
  _aTrousPingImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_B10G11R11_UFLOAT_PACK32,
                              VK_IMAGE_USAGE_STORAGE_BIT);

  // also serves as the output image
  _aTrousPongImage =
      std::make_unique<Image>(_lowResWidth, _lowResHeight, 1, VK_FORMAT_B10G11R11_UFLOAT_PACK32,
                              VK_IMAGE_USAGE_STORAGE_BIT);

  _renderTargetImage = std::make_unique<Image>(
      _highResWidth, _highResHeight, 1, VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
}

void SvoTracer::_createImageForwardingPairs() {
  _normalForwardingPair = std::make_unique<ImageForwardingPair>(
      _normalImage->getVkImage(), _lastNormalImage->getVkImage(), _lowResWidth, _lowResHeight,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  _positionForwardingPair = std::make_unique<ImageForwardingPair>(
      _positionImage->getVkImage(), _lastPositionImage->getVkImage(), _lowResWidth, _lowResHeight,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  _voxHashForwardingPair = std::make_unique<ImageForwardingPair>(
      _voxHashImage->getVkImage(), _lastVoxHashImage->getVkImage(), _lowResWidth, _lowResHeight,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  _accumedForwardingPair = std::make_unique<ImageForwardingPair>(
      _accumedImage->getVkImage(), _lastAccumedImage->getVkImage(), _lowResWidth, _lowResHeight,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  _godRayAccumedForwardingPair = std::make_unique<ImageForwardingPair>(
      _godRayAccumedImage->getVkImage(), _lastGodRayAccumedImage->getVkImage(), _lowResWidth,
      _lowResHeight, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  _taaForwardingPair = std::make_unique<ImageForwardingPair>(
      _taaImage->getVkImage(), _lastTaaImage->getVkImage(), _highResWidth, _highResHeight,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_GENERAL);

  // creating forwarding pairs to copy the image result each frame to a specific swapchain
  _targetForwardingPairs.clear();
  for (int i = 0; i < _appContext->getSwapchainImagesCount(); i++) {
    _targetForwardingPairs.emplace_back(std::make_unique<ImageForwardingPair>(
        _renderTargetImage->getVkImage(), _appContext->getSwapchainImages()[i], _highResWidth,
        _highResHeight, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  }
}

// these buffers are modified by the CPU side every frame, and we have multiple frames in flight,
// so we need to create multiple copies of them, they are fairly small though
void SvoTracer::_createBuffersAndBufferBundles() {
  // buffers
  _sceneInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_SceneInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  _aTrousIterationBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      MemoryStyle::kDedicated);

  _aTrousIterationStagingBuffers.clear();
  _aTrousIterationStagingBuffers.reserve(_aTrousSizeMax);
  for (int i = 0; i < _aTrousSizeMax; i++) {
    _aTrousIterationStagingBuffers.emplace_back(std::make_unique<Buffer>(
        sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryStyle::kDedicated));
  }

  _outputInfoBuffer = std::make_unique<Buffer>(
      sizeof(G_OutputInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryStyle::kDedicated);

  // buffer bundles
  _renderInfoBufferBundle =
      std::make_unique<BufferBundle>(_framesInFlight, sizeof(G_RenderInfo),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, MemoryStyle::kHostVisible);

  _environmentInfoBufferBundle =
      std::make_unique<BufferBundle>(_framesInFlight, sizeof(G_EnvironmentInfo),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, MemoryStyle::kHostVisible);

  _tweakableParametersBufferBundle =
      std::make_unique<BufferBundle>(_framesInFlight, sizeof(G_TweakableParameters),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, MemoryStyle::kHostVisible);

  _temporalFilterInfoBufferBundle =
      std::make_unique<BufferBundle>(_framesInFlight, sizeof(G_TemporalFilterInfo),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, MemoryStyle::kHostVisible);

  _spatialFilterInfoBufferBundle =
      std::make_unique<BufferBundle>(_framesInFlight, sizeof(G_SpatialFilterInfo),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, MemoryStyle::kHostVisible);
}

void SvoTracer::_initBufferData() {
  G_SceneInfo sceneData = {_beamResolution, _svoBuilder->getVoxelLevelCount(),
                           _svoBuilder->getChunksDim()};
  _sceneInfoBuffer->fillData(&sceneData);

  for (uint32_t i = 0; i < _aTrousSizeMax; i++) {
    uint32_t aTrousIteration = i;
    _aTrousIterationStagingBuffers[i]->fillData(&aTrousIteration);
  }
}

void SvoTracer::_recordRenderingCommandBuffers() {
  for (auto &commandBuffer : _tracingCommandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }
  _tracingCommandBuffers.clear();

  _tracingCommandBuffers.resize(_framesInFlight); //  change this later on, because it is
                                                  //  bounded to the swapchain image
  VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool        = _appContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)_tracingCommandBuffers.size();

  vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, _tracingCommandBuffers.data());

  VkMemoryBarrier uboWritingBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  uboWritingBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
  uboWritingBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  // create the general memory barrier
  VkMemoryBarrier memoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

  for (uint32_t frameIndex = 0; frameIndex < _tracingCommandBuffers.size(); frameIndex++) {
    auto &cmdBuffer = _tracingCommandBuffers[frameIndex];

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    // make all host writes to the ubo visible to the shaders
    vkCmdPipelineBarrier(cmdBuffer,
                         VK_PIPELINE_STAGE_HOST_BIT,           // source stage
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // destination stage
                         0,                                    // dependency flags
                         1,                                    // memory barrier count
                         &uboWritingBarrier,                   // memory barriers
                         0,                                    // buffer memory barrier count
                         nullptr,                              // buffer memory barriers
                         0,                                    // image memory barrier count
                         nullptr                               // image memory barriers
    );

    // _renderTargetImage->clearImage(cmdBuffer);
    _transmittanceLutPipeline->recordCommand(cmdBuffer, frameIndex, kTransmittanceLutWidth,
                                             kTransmittanceLutHeight, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _multiScatteringLutPipeline->recordCommand(cmdBuffer, frameIndex, kMultiScatteringLutWidth,
                                               kMultiScatteringLutHeight, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _skyViewLutPipeline->recordCommand(cmdBuffer, frameIndex, kSkyViewLutWidth, kSkyViewLutHeight,
                                       1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _shadowMapPipeline->recordCommand(cmdBuffer, frameIndex, _shadowMapResolution,
                                      _shadowMapResolution, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _svoCourseBeamPipeline->recordCommand(
        cmdBuffer, frameIndex,
        static_cast<uint32_t>(
            std::ceil(static_cast<float>(_lowResWidth) / static_cast<float>(_beamResolution))) +
            1,
        static_cast<uint32_t>(
            std::ceil(static_cast<float>(_lowResHeight) / static_cast<float>(_beamResolution))) +
            1,
        1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _svoTracingPipeline->recordCommand(cmdBuffer, frameIndex, _lowResWidth, _lowResHeight, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _temporalFilterPipeline->recordCommand(cmdBuffer, frameIndex, _lowResWidth, _lowResHeight, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    for (int i = 0; i < _aTrousSizeMax; i++) {
      VkBufferCopy bufCopy = {
          0,                                 // srcOffset
          0,                                 // dstOffset,
          _aTrousIterationBuffer->getSize(), // size
      };

      vkCmdCopyBuffer(cmdBuffer, _aTrousIterationStagingBuffers[i]->getVkBuffer(),
                      _aTrousIterationBuffer->getVkBuffer(), 1, &bufCopy);

      VkMemoryBarrier bufferCopyMemoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
      bufferCopyMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      bufferCopyMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(cmdBuffer,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,       // source stage
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // destination stage
                           0,                                    // dependency flags
                           1,                                    // memory barrier count
                           &bufferCopyMemoryBarrier,             // memory barriers
                           0,                                    // buffer memory barrier count
                           nullptr,                              // buffer memory barriers
                           0,                                    // image memory barrier count
                           nullptr                               // image memory barriers
      );

      _aTrousPipeline->recordCommand(cmdBuffer, frameIndex, _lowResWidth, _lowResHeight, 1);

      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr,
                           0, nullptr);
    }

    _backgroundBlitPipeline->recordCommand(cmdBuffer, frameIndex, _lowResWidth, _lowResHeight, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _taaUpscalingPipeline->recordCommand(cmdBuffer, frameIndex, _highResWidth, _highResHeight, 1);

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0,
                         nullptr);

    _postProcessingPipeline->recordCommand(cmdBuffer, frameIndex, _highResWidth, _highResHeight, 1);

    // copy to history images
    _normalForwardingPair->forwardCopy(cmdBuffer);
    _positionForwardingPair->forwardCopy(cmdBuffer);
    _voxHashForwardingPair->forwardCopy(cmdBuffer);
    _accumedForwardingPair->forwardCopy(cmdBuffer);
    _godRayAccumedForwardingPair->forwardCopy(cmdBuffer);
    _taaForwardingPair->forwardCopy(cmdBuffer);

    vkEndCommandBuffer(cmdBuffer);
  }
}

void SvoTracer::_recordDeliveryCommandBuffers() {
  for (auto &commandBuffer : _deliveryCommandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                         &commandBuffer);
  }
  _deliveryCommandBuffers.clear();

  _deliveryCommandBuffers.resize(_appContext->getSwapchainImagesCount());

  VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool        = _appContext->getCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(_deliveryCommandBuffers.size());

  vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, _deliveryCommandBuffers.data());

  VkMemoryBarrier deliveryMemoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  deliveryMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  deliveryMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  for (size_t imageIndex = 0; imageIndex < _deliveryCommandBuffers.size(); imageIndex++) {
    auto &cmdBuffer = _deliveryCommandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    // // make all host writes to the ubo visible to the shaders
    // vkCmdPipelineBarrier(cmdBuffer,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // source stage
    //                      VK_PIPELINE_STAGE_TRANSFER_BIT,       // destination stage
    //                      0,                                    // dependency flags
    //                      1,                                    // memory barrier count
    //                      &deliveryMemoryBarrier,               // memory barriers
    //                      0,                                    // buffer memory barrier count
    //                      nullptr,                              // buffer memory barriers
    //                      0,                                    // image memory barrier count
    //                      nullptr                               // image memory barriers
    // );

    _targetForwardingPairs[imageIndex]->forwardCopy(cmdBuffer);
    vkEndCommandBuffer(cmdBuffer);
  }
}

void SvoTracer::drawFrame(size_t currentFrame) {
  _updateShadowMapCamera();
  _updateUboData(currentFrame);
}

void SvoTracer::_updateShadowMapCamera() {
  glm::vec3 sunDir = _getSunDir(_tweakingData.sunAltitude, _tweakingData.sunAzimuth);
  _shadowMapCamera->updateCameraVectors(_camera->getPosition(), sunDir);
}

void SvoTracer::_updateUboData(size_t currentFrame) {
  static uint32_t currentSample = 0;
  // identity matrix
  static glm::mat4 vMatPrev{1.0F};
  static glm::mat4 vMatPrevInv{1.0F};
  static glm::mat4 pMatPrev{1.0F};
  static glm::mat4 pMatPrevInv{1.0F};
  static glm::mat4 vpMatPrev{1.0F};
  static glm::mat4 vpMatPrevInv{1.0F};

  auto currentTime = static_cast<float>(glfwGetTime());

  auto vMat    = _camera->getViewMatrix();
  auto vMatInv = glm::inverse(vMat);
  auto pMat =
      _camera->getProjectionMatrix(static_cast<float>(_appContext->getSwapchainExtentWidth()) /
                                   static_cast<float>(_appContext->getSwapchainExtentHeight()));
  auto pMatInv  = glm::inverse(pMat);
  auto vpMat    = pMat * vMat;
  auto vpMatInv = glm::inverse(vpMat);

  auto vpMatShadowMapCam =
      _shadowMapCamera->getProjectionMatrix() * _shadowMapCamera->getViewMatrix();
  auto vpMatShadowMapCamInv = glm::inverse(vpMatShadowMapCam);

  G_RenderInfo renderInfo = {
      _camera->getPosition(),
      _shadowMapCamera->getPosition(),
      _subpixOffsets[currentSample % _taaSamplingOffsetSize],
      vMat,
      vMatInv,
      vMatPrev,
      vMatPrevInv,
      pMat,
      pMatInv,
      pMatPrev,
      pMatPrevInv,
      vpMat,
      vpMatInv,
      vpMatPrev,
      vpMatPrevInv,
      vpMatShadowMapCam,
      vpMatShadowMapCamInv,
      glm::uvec2(_lowResWidth, _lowResHeight),
      glm::vec2(1.F / static_cast<float>(_lowResWidth), 1.F / static_cast<float>(_lowResHeight)),
      glm::uvec2(_highResWidth, _highResHeight),
      glm::vec2(1.F / static_cast<float>(_highResWidth), 1.F / static_cast<float>(_highResHeight)),
      _camera->getVFov(),
      currentSample,
      currentTime,
  };
  _renderInfoBufferBundle->getBuffer(currentFrame)->fillData(&renderInfo);

  vMatPrev     = vMat;
  vMatPrevInv  = vMatInv;
  pMatPrev     = pMat;
  pMatPrevInv  = pMatInv;
  vpMatPrev    = vpMat;
  vpMatPrevInv = vpMatInv;

  glm::vec3 sunDir = _getSunDir(_tweakingData.sunAltitude, _tweakingData.sunAzimuth);
  G_EnvironmentInfo environmentInfo{};
  environmentInfo.sunDir                 = sunDir;
  environmentInfo.rayleighScatteringBase = _tweakingData.rayleighScatteringBase;
  environmentInfo.mieScatteringBase      = _tweakingData.mieScatteringBase;
  environmentInfo.mieAbsorptionBase      = _tweakingData.mieAbsorptionBase;
  environmentInfo.ozoneAbsorptionBase    = _tweakingData.ozoneAbsorptionBase;
  environmentInfo.sunLuminance           = _tweakingData.sunLuminance;
  environmentInfo.atmosLuminance         = _tweakingData.atmosLuminance;
  environmentInfo.sunSize                = _tweakingData.sunSize;
  _environmentInfoBufferBundle->getBuffer(currentFrame)->fillData(&environmentInfo);

  G_TweakableParameters tweakableParameters{};
  tweakableParameters.debugB1          = _tweakingData.debugB1;
  tweakableParameters.debugF1          = _tweakingData.debugF1;
  tweakableParameters.debugI1          = _tweakingData.debugI1;
  tweakableParameters.visualizeChunks  = _tweakingData.visualizeChunks;
  tweakableParameters.visualizeOctree  = _tweakingData.visualizeOctree;
  tweakableParameters.beamOptimization = _tweakingData.beamOptimization;
  tweakableParameters.traceIndirectRay = _tweakingData.traceIndirectRay;
  tweakableParameters.taa              = _tweakingData.taa;
  _tweakableParametersBufferBundle->getBuffer(currentFrame)->fillData(&tweakableParameters);

  G_TemporalFilterInfo temporalFilterInfo{};
  temporalFilterInfo.temporalAlpha       = _tweakingData.temporalAlpha;
  temporalFilterInfo.temporalPositionPhi = _tweakingData.temporalPositionPhi;
  _temporalFilterInfoBufferBundle->getBuffer(currentFrame)->fillData(&temporalFilterInfo);

  G_SpatialFilterInfo spatialFilterInfo{};
  spatialFilterInfo.aTrousIterationCount =
      static_cast<uint32_t>(_tweakingData.aTrousIterationCount);
  spatialFilterInfo.phiC                  = _tweakingData.phiC;
  spatialFilterInfo.phiN                  = _tweakingData.phiN;
  spatialFilterInfo.phiP                  = _tweakingData.phiP;
  spatialFilterInfo.minPhiZ               = _tweakingData.minPhiZ;
  spatialFilterInfo.maxPhiZ               = _tweakingData.maxPhiZ;
  spatialFilterInfo.phiZStableSampleCount = _tweakingData.phiZStableSampleCount;
  spatialFilterInfo.changingLuminancePhi  = _tweakingData.changingLuminancePhi;
  _spatialFilterInfoBufferBundle->getBuffer(currentFrame)->fillData(&spatialFilterInfo);

  currentSample++;
}

G_OutputInfo SvoTracer::getOutputInfo() {
  G_OutputInfo outputInfo{};
  _outputInfoBuffer->fetchData(&outputInfo);
  return outputInfo;
}

void SvoTracer::_createDescriptorSetBundle() {
  _descriptorSetBundle = std::make_unique<DescriptorSetBundle>(_appContext, _framesInFlight,
                                                               VK_SHADER_STAGE_COMPUTE_BIT);

  _descriptorSetBundle->bindUniformBufferBundle(0, _renderInfoBufferBundle.get());
  _descriptorSetBundle->bindUniformBufferBundle(1, _environmentInfoBufferBundle.get());
  _descriptorSetBundle->bindUniformBufferBundle(2, _tweakableParametersBufferBundle.get());
  _descriptorSetBundle->bindUniformBufferBundle(3, _temporalFilterInfoBufferBundle.get());
  _descriptorSetBundle->bindUniformBufferBundle(4, _spatialFilterInfoBufferBundle.get());

  _descriptorSetBundle->bindStorageImage(48, _scalarBlueNoise.get());
  _descriptorSetBundle->bindStorageImage(5, _vec2BlueNoise.get());
  _descriptorSetBundle->bindStorageImage(42, _vec3BlueNoise.get());
  _descriptorSetBundle->bindStorageImage(6, _weightedCosineBlueNoise.get());

  _descriptorSetBundle->bindStorageImage(7, _svoBuilder->getChunksImage());
  _descriptorSetBundle->bindStorageImage(8, _backgroundImage.get());
  _descriptorSetBundle->bindStorageImage(9, _beamDepthImage.get());
  _descriptorSetBundle->bindStorageImage(10, _rawImage.get());
  _descriptorSetBundle->bindStorageImage(45, _godRayImage.get());
  _descriptorSetBundle->bindStorageImage(11, _depthImage.get());
  _descriptorSetBundle->bindStorageImage(12, _octreeVisualizationImage.get());
  _descriptorSetBundle->bindStorageImage(13, _hitImage.get());
  _descriptorSetBundle->bindStorageImage(14, _temporalHistLengthImage.get());
  _descriptorSetBundle->bindStorageImage(15, _motionImage.get());
  _descriptorSetBundle->bindStorageImage(16, _normalImage.get());
  _descriptorSetBundle->bindStorageImage(17, _lastNormalImage.get());
  _descriptorSetBundle->bindStorageImage(18, _positionImage.get());
  _descriptorSetBundle->bindStorageImage(19, _lastPositionImage.get());
  _descriptorSetBundle->bindStorageImage(20, _voxHashImage.get());
  _descriptorSetBundle->bindStorageImage(21, _lastVoxHashImage.get());
  _descriptorSetBundle->bindStorageImage(22, _accumedImage.get());
  _descriptorSetBundle->bindStorageImage(23, _lastAccumedImage.get());
  _descriptorSetBundle->bindStorageImage(46, _godRayAccumedImage.get());
  _descriptorSetBundle->bindStorageImage(47, _lastGodRayAccumedImage.get());

  _descriptorSetBundle->bindStorageImage(24, _taaImage.get());
  _descriptorSetBundle->bindStorageImage(25, _lastTaaImage.get());

  _descriptorSetBundle->bindStorageImage(26, _blittedImage.get());

  _descriptorSetBundle->bindStorageImage(27, _aTrousPingImage.get());
  _descriptorSetBundle->bindStorageImage(28, _aTrousPongImage.get());

  _descriptorSetBundle->bindStorageImage(30, _renderTargetImage.get());

  _descriptorSetBundle->bindImageSampler(31, _lastTaaImage.get());

  _descriptorSetBundle->bindStorageImage(36, _transmittanceLutImage.get());
  _descriptorSetBundle->bindStorageImage(37, _multiScatteringLutImage.get());
  _descriptorSetBundle->bindStorageImage(38, _skyViewLutImage.get());
  _descriptorSetBundle->bindImageSampler(39, _transmittanceLutImage.get());
  _descriptorSetBundle->bindImageSampler(40, _multiScatteringLutImage.get());
  _descriptorSetBundle->bindImageSampler(41, _skyViewLutImage.get());

  _descriptorSetBundle->bindStorageImage(43, _shadowMapImage.get());
  _descriptorSetBundle->bindImageSampler(44, _shadowMapImage.get());

  _descriptorSetBundle->bindStorageBuffer(32, _sceneInfoBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(33, _svoBuilder->getAppendedOctreeBuffer());
  _descriptorSetBundle->bindStorageBuffer(34, _aTrousIterationBuffer.get());
  _descriptorSetBundle->bindStorageBuffer(35, _outputInfoBuffer.get());

  _descriptorSetBundle->create();
}

void SvoTracer::_createPipelines() {
  _transmittanceLutPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("transmittanceLut.comp"),
      WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _transmittanceLutPipeline->compileAndCacheShaderModule(false);
  _transmittanceLutPipeline->build();

  _multiScatteringLutPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("multiScatteringLut.comp"),
      WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _multiScatteringLutPipeline->compileAndCacheShaderModule(false);
  _multiScatteringLutPipeline->build();

  _skyViewLutPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("skyViewLut.comp"), WorkGroupSize{8, 8, 1},
      _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _skyViewLutPipeline->compileAndCacheShaderModule(false);
  _skyViewLutPipeline->build();

  _shadowMapPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("shadowMap.comp"), WorkGroupSize{8, 8, 1},
      _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _shadowMapPipeline->compileAndCacheShaderModule(false);
  _shadowMapPipeline->build();

  _svoCourseBeamPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("svoCoarseBeam.comp"), WorkGroupSize{8, 8, 1},
      _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _svoCourseBeamPipeline->compileAndCacheShaderModule(false);
  _svoCourseBeamPipeline->build();

  _svoTracingPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("svoTracing.comp"), WorkGroupSize{8, 8, 1},
      _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _svoTracingPipeline->compileAndCacheShaderModule(false);
  _svoTracingPipeline->build();

  _temporalFilterPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("temporalFilter.comp"),
      WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _temporalFilterPipeline->compileAndCacheShaderModule(false);
  _temporalFilterPipeline->build();

  _aTrousPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("aTrous.comp"), WorkGroupSize{8, 8, 1},
      _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _aTrousPipeline->compileAndCacheShaderModule(false);
  _aTrousPipeline->build();

  _backgroundBlitPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("backgroundBlit.comp"),
      WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _backgroundBlitPipeline->compileAndCacheShaderModule(false);
  _backgroundBlitPipeline->build();

  _taaUpscalingPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("taaUpscaling.comp"), WorkGroupSize{8, 8, 1},
      _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _taaUpscalingPipeline->compileAndCacheShaderModule(false);
  _taaUpscalingPipeline->build();

  _postProcessingPipeline = std::make_unique<ComputePipeline>(
      _appContext, _logger, this, _makeShaderFullPath("postProcessing.comp"),
      WorkGroupSize{8, 8, 1}, _descriptorSetBundle.get(), _shaderCompiler, _shaderChangeListener);
  _postProcessingPipeline->compileAndCacheShaderModule(false);
  _postProcessingPipeline->build();
}

void SvoTracer::_updatePipelinesDescriptorBundles() {
  _transmittanceLutPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
  _multiScatteringLutPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
  _skyViewLutPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());

  _shadowMapPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());

  _svoCourseBeamPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
  _svoTracingPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
  _temporalFilterPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
  _aTrousPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
  _backgroundBlitPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
  _taaUpscalingPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
  _postProcessingPipeline->updateDescriptorSetBundle(_descriptorSetBundle.get());
}
