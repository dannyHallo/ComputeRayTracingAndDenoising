#include "ModelsHolder.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "application/app-res/buffers/BuffersHolder.hpp"
#include "application/app-res/images/ImagesHolder.hpp"

#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <map>

namespace {
std::string _makeShaderPath(const std::string &shaderName) {
  return kPathToResourceFolder + "/shaders/generated/" + shaderName + ".spv";
}
} // namespace

static const std::map<std::string, WorkGroupSize> kShaderNameToWorkGroupSizes = {
    {"gradientProjection", {8, 8, 1}}, {"trisTracing", {8, 8, 1}},
    {"svoTracing", {8, 8, 1}},         {"screenSpaceGradient", {32, 32, 1}},
    {"stratumFilter", {32, 32, 1}},    {"temporalFilter", {32, 32, 1}},
    {"variance", {32, 32, 1}},         {"aTrous", {32, 32, 1}},
    {"postProcessing", {8, 8, 1}},
};

// currently unused
void ModelsHolder::_createGradientProjectionModel() {
  // mat creation
  std::string const shaderName = "gradientProjection";
  auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

  // ubo
  mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
  mat->addUniformBufferBundle(_buffersHolder->getGradientProjectionBufferBundle());

  // image read
  mat->addStorageImage(_imagesHolder->getVec2BlueNoise());
  mat->addStorageImage(_imagesHolder->getWeightedCosineBlueNoise());
  mat->addStorageImage(_imagesHolder->getRawImage());
  mat->addStorageImage(_imagesHolder->getPositionImage());
  mat->addStorageImage(_imagesHolder->getSeedImage());
  // image write
  mat->addStorageImage(_imagesHolder->getStratumOffsetImage());
  // (atomic) readwrite
  mat->addStorageImage(_imagesHolder->getPerStratumLockingImage());
  // image write
  mat->addStorageImage(_imagesHolder->getVisibilityImage());
  mat->addStorageImage(_imagesHolder->getSeedVisibilityImage());

  // model creation
  _gradientProjectionModel = std::make_unique<ComputeModel>(
      std::move(mat), _framesInFlight, kShaderNameToWorkGroupSizes.at(shaderName));
}

// currently unused
void ModelsHolder::_createRtxModel() {
  // mat creation
  std::string const shaderName = "trisTracing";
  auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

  // ubo
  mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
  mat->addUniformBufferBundle(_buffersHolder->getRtxBufferBundle());

  // image read
  mat->addStorageImage(_imagesHolder->getVec2BlueNoise());
  mat->addStorageImage(_imagesHolder->getWeightedCosineBlueNoise());
  mat->addStorageImage(_imagesHolder->getStratumOffsetImage());
  mat->addStorageImage(_imagesHolder->getVisibilityImage());
  mat->addStorageImage(_imagesHolder->getSeedVisibilityImage());

  // image write
  mat->addStorageImage(_imagesHolder->getPositionImage());
  mat->addStorageImage(_imagesHolder->getNormalImage());
  mat->addStorageImage(_imagesHolder->getDepthImage());
  mat->addStorageImage(_imagesHolder->getMeshHashImage());
  mat->addStorageImage(_imagesHolder->getRawImage());
  mat->addStorageImage(_imagesHolder->getSeedImage());
  mat->addStorageImage(_imagesHolder->getTemporalGradientNormalizationImagePing());

  // buffers
  mat->addStorageBufferBundle(_buffersHolder->getTriangleBufferBundle());
  mat->addStorageBufferBundle(_buffersHolder->getMaterialBufferBundle());
  mat->addStorageBufferBundle(_buffersHolder->getBvhBufferBundle());
  mat->addStorageBufferBundle(_buffersHolder->getLightsBufferBundle());

  // model creation
  _rtxModel = std::make_unique<ComputeModel>(std::move(mat), _framesInFlight,
                                             kShaderNameToWorkGroupSizes.at(shaderName));
}

void ModelsHolder::_createSvoModel() {
  // mat creation
  std::string const shaderName = "svoTracing";
  auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

  // ubo
  mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());

  // image write
  mat->addStorageImage(_imagesHolder->getRawImage());

  // buffers
  mat->addStorageBufferBundle(_buffersHolder->getSvoBufferBundle());

  // model creation
  _svoModel = std::make_unique<ComputeModel>(std::move(mat), _framesInFlight,
                                             kShaderNameToWorkGroupSizes.at(shaderName));
}

// currently unused
void ModelsHolder::_createGradientModel() {
  // mat creation
  std::string const shaderName = "screenSpaceGradient";
  auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

  // ubo
  mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());

  // image read
  mat->addStorageImage(_imagesHolder->getDepthImage());

  // image write
  mat->addStorageImage(_imagesHolder->getGradientImage());

  // model creation
  _gradientModel = std::make_unique<ComputeModel>(std::move(mat), _framesInFlight,
                                                  kShaderNameToWorkGroupSizes.at(shaderName));
}

// currently unused
void ModelsHolder::_createStratumFilterModels() {
  _stratumFilterModels.clear();
  for (int i = 0; i < _stratumFilterSize; i++) {
    // mat creation
    std::string const shaderName = "stratumFilter";
    auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

    // ubo
    mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
    mat->addUniformBufferBundle(_buffersHolder->getStratumFilterBufferBundle(i));

    // image read
    mat->addStorageImage(_imagesHolder->getPositionImage());
    mat->addStorageImage(_imagesHolder->getNormalImage());
    mat->addStorageImage(_imagesHolder->getDepthImage());
    mat->addStorageImage(_imagesHolder->getGradientImage());
    mat->addStorageImage(_imagesHolder->getRawImage());
    mat->addStorageImage(_imagesHolder->getStratumOffsetImage());

    // pingpong
    mat->addStorageImage(_imagesHolder->getTemporalGradientNormalizationImagePing());
    mat->addStorageImage(_imagesHolder->getTemporalGradientNormalizationImagePong());

    // model creation
    _stratumFilterModels.emplace_back(std::make_unique<ComputeModel>(
        std::move(mat), _framesInFlight, kShaderNameToWorkGroupSizes.at(shaderName)));
  }
}

// currently unused
void ModelsHolder::_createTemporalFilterModel() {
  // mat creation
  std::string const shaderName = "temporalFilter";
  auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

  // ubo
  mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
  mat->addUniformBufferBundle(_buffersHolder->getTemperalFilterBufferBundle());

  // image read
  mat->addStorageImage(_imagesHolder->getPositionImage());
  mat->addStorageImage(_imagesHolder->getRawImage());
  mat->addStorageImage(_imagesHolder->getDepthImage());
  mat->addStorageImage(_imagesHolder->getDepthImagePrev());
  mat->addStorageImage(_imagesHolder->getNormalImage());
  mat->addStorageImage(_imagesHolder->getNormalImagePrev());
  mat->addStorageImage(_imagesHolder->getGradientImage());
  mat->addStorageImage(_imagesHolder->getGradientImagePrev());
  mat->addStorageImage(_imagesHolder->getMeshHashImage());
  mat->addStorageImage(_imagesHolder->getMeshHashImagePrev());
  mat->addStorageImage(_imagesHolder->getLastFrameAccumImage());
  mat->addStorageImage(_imagesHolder->getVarianceHistImagePrev());

  // image write
  mat->addStorageImage(_imagesHolder->getATrousInputImage());
  mat->addStorageImage(_imagesHolder->getVarianceHistImage());

  // model creation
  _temporalFilterModel = std::make_unique<ComputeModel>(std::move(mat), _framesInFlight,
                                                        kShaderNameToWorkGroupSizes.at(shaderName));
}

// currently unused
void ModelsHolder::_createVarianceModel() {
  // mat creation
  std::string const shaderName = "variance";
  auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

  // ubo
  mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
  mat->addUniformBufferBundle(_buffersHolder->getVarianceBufferBundle());

  // image read
  mat->addStorageImage(_imagesHolder->getATrousInputImage());
  mat->addStorageImage(_imagesHolder->getNormalImage());
  mat->addStorageImage(_imagesHolder->getDepthImage());

  // image write
  mat->addStorageImage(_imagesHolder->getGradientImage());
  mat->addStorageImage(_imagesHolder->getVarianceImage());
  mat->addStorageImage(_imagesHolder->getVarianceHistImage());

  // model creation
  _varianceModel = std::make_unique<ComputeModel>(std::move(mat), _framesInFlight,
                                                  kShaderNameToWorkGroupSizes.at(shaderName));
}

// currently unused
void ModelsHolder::_createATrousModels() {
  _aTrousModels.clear();
  for (int i = 0; i < _aTrousSize; i++) {
    // mat creation
    std::string const shaderName = "aTrous";
    auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

    // ubo
    mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
    mat->addUniformBufferBundle(_buffersHolder->getBlurFilterBufferBundle(i));

    // image read
    mat->addStorageImage(_imagesHolder->getVec2BlueNoise());
    mat->addStorageImage(_imagesHolder->getWeightedCosineBlueNoise());
    mat->addStorageImage(_imagesHolder->getATrousInputImage());
    mat->addStorageImage(_imagesHolder->getNormalImage());
    mat->addStorageImage(_imagesHolder->getDepthImage());
    mat->addStorageImage(_imagesHolder->getGradientImage());
    mat->addStorageImage(_imagesHolder->getVarianceImage());

    // image write
    mat->addStorageImage(_imagesHolder->getLastFrameAccumImage());
    mat->addStorageImage(_imagesHolder->getATrousOutputImage());

    // model creation
    _aTrousModels.emplace_back(std::make_unique<ComputeModel>(
        std::move(mat), _framesInFlight, kShaderNameToWorkGroupSizes.at(shaderName)));
  }
}

void ModelsHolder::_createPostProcessingModel() {
  // mat creation
  std::string const shaderName = "postProcessing";
  auto postProcessingMat =
      std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));

  // ubo
  postProcessingMat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
  postProcessingMat->addUniformBufferBundle(_buffersHolder->getPostProcessingBufferBundle());

  // image read
  postProcessingMat->addStorageImage(_imagesHolder->getATrousOutputImage());
  postProcessingMat->addStorageImage(_imagesHolder->getVarianceImage());
  postProcessingMat->addStorageImage(_imagesHolder->getRawImage());
  postProcessingMat->addStorageImage(_imagesHolder->getStratumOffsetImage());
  postProcessingMat->addStorageImage(_imagesHolder->getVisibilityImage());
  postProcessingMat->addStorageImage(_imagesHolder->getTemporalGradientNormalizationImagePong());
  postProcessingMat->addStorageImage(_imagesHolder->getSeedImage());

  // image write
  postProcessingMat->addStorageImage(_imagesHolder->getTargetImage());

  // model creation
  _postProcessingModel = std::make_unique<ComputeModel>(
      std::move(postProcessingMat), _framesInFlight, kShaderNameToWorkGroupSizes.at(shaderName));
}

void ModelsHolder::init(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder,
                        int stratumFilterSize, int aTrousSize, size_t framesInFlight) {

  _imagesHolder  = imagesHolder;
  _buffersHolder = buffersHolder;

  _stratumFilterSize = stratumFilterSize;
  _aTrousSize        = aTrousSize;
  _framesInFlight    = framesInFlight;

  // _createGradientProjectionModel();
  // _createRtxModel();
  _createSvoModel();
  // _createGradientModel();
  // _createStratumFilterModels();
  // _createTemporalFilterModel();
  // _createVarianceModel();
  // _createATrousModels();
  _createPostProcessingModel();
}