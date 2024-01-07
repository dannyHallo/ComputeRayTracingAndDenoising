#include "ModelsHolder.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "application/app-res/buffers/BuffersHolder.hpp"
#include "application/app-res/images/ImagesHolder.hpp"

#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <map>

ModelsHolder::ModelsHolder(VulkanApplicationContext *appContext, Logger *logger)
    : _appContext(appContext), _logger(logger){};

namespace {
std::string _makeShaderPath(const std::string &shaderName) {
  return kPathToResourceFolder + "/shaders/generated/" + shaderName + ".spv";
}
} // namespace

static const std::map<std::string, WorkGroupSize> kShaderNameToWorkGroupSizes = {
    {"rtx", {8, 8, 1}},
    // {"screenSpaceGradient", {32, 32, 1}},
    // {"stratumFilter", {32, 32, 1}},
    // {"temporalFilter", {32, 32, 1}},
    // {"variance", {32, 32, 1}},
    // {"aTrous", {32, 32, 1}},
    {"postProcessing", {8, 8, 1}},
};

// void ModelsHolder::_createGradientProjectionModel(ImagesHolder *imagesHolder,
//                                                   BuffersHolder *buffersHolder) {
//   auto gradientProjectionMat =
//       std::make_unique<ComputeMaterial>(_appContext,  _logger,
//       _makeShaderPath("gradientProjection"));
//   gradientProjectionMat->addUniformBufferBundle(buffersHolder->getGlobalBufferBundle());
//   gradientProjectionMat->addUniformBufferBundle(buffersHolder->getGradientProjectionBufferBundle());
//   // read
//   gradientProjectionMat->addStorageImage(imagesHolder->getVec2BlueNoise());
//   gradientProjectionMat->addStorageImage(imagesHolder->getWeightedCosineBlueNoise());
//   gradientProjectionMat->addStorageImage(imagesHolder->getRawImage());
//   gradientProjectionMat->addStorageImage(imagesHolder->getPositionImage());
//   gradientProjectionMat->addStorageImage(imagesHolder->getSeedImage());
//   // write
//   gradientProjectionMat->addStorageImage(imagesHolder->getStratumOffsetImage());
//   // (atomic) readwrite
//   gradientProjectionMat->addStorageImage(imagesHolder->getPerStratumLockingImage());
//   // write
//   gradientProjectionMat->addStorageImage(imagesHolder->getVisibilityImage());
//   gradientProjectionMat->addStorageImage(imagesHolder->getSeedVisibilityImage());

//   _gradientProjectionModel =
//       std::make_unique<ComputeModel>(_logger, std::move(gradientProjectionMat), 24, 24, 24);
// }

void ModelsHolder::_createRtxModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder,
                                   size_t framesInFlight) {
  std::string const shaderName = "rtx";
  auto rtxMat =
      std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));
  rtxMat->addUniformBufferBundle(buffersHolder->getGlobalBufferBundle());
  rtxMat->addUniformBufferBundle(buffersHolder->getRtxBufferBundle());
  // read
  rtxMat->addStorageImage(imagesHolder->getVec2BlueNoise());
  rtxMat->addStorageImage(imagesHolder->getWeightedCosineBlueNoise());
  rtxMat->addStorageImage(imagesHolder->getStratumOffsetImage());
  rtxMat->addStorageImage(imagesHolder->getVisibilityImage());
  rtxMat->addStorageImage(imagesHolder->getSeedVisibilityImage());
  // output
  rtxMat->addStorageImage(imagesHolder->getPositionImage());
  rtxMat->addStorageImage(imagesHolder->getNormalImage());
  rtxMat->addStorageImage(imagesHolder->getDepthImage());
  rtxMat->addStorageImage(imagesHolder->getMeshHashImage());
  rtxMat->addStorageImage(imagesHolder->getRawImage());
  rtxMat->addStorageImage(imagesHolder->getSeedImage());
  rtxMat->addStorageImage(imagesHolder->getTemporalGradientNormalizationImagePing());
  // buffers
  rtxMat->addStorageBufferBundle(buffersHolder->getTriangleBufferBundle());
  rtxMat->addStorageBufferBundle(buffersHolder->getMaterialBufferBundle());
  rtxMat->addStorageBufferBundle(buffersHolder->getBvhBufferBundle());
  rtxMat->addStorageBufferBundle(buffersHolder->getLightsBufferBundle());
  _rtxModel = std::make_unique<ComputeModel>(std::move(rtxMat), framesInFlight,
                                             kShaderNameToWorkGroupSizes.at(shaderName));
}

// void ModelsHolder::_createGradientModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder)
// {

//   auto gradientMat =
//       std::make_unique<ComputeMaterial>(appContext, _logger,
//       _makeShaderPath("screenSpaceGradient"));
//   gradientMat->addUniformBufferBundle(buffersHolder->getGlobalBufferBundle());
//   // i2nput
//   gradientMat->addStorageImage(imagesHolder->getDepthImage());
//   // output
//   gradientMat->addStorageImage(imagesHolder->getGradientImage());
//   _gradientModel = std::make_unique<ComputeModel>(std::move(gradientMat), 32, 32, 32);
// }

// void ModelsHolder::_createStratumFilterModels(ImagesHolder *imagesHolder,
//                                               BuffersHolder *buffersHolder) {
//   _stratumFilterModels.clear();
//   for (int i = 0; i < 6; i++) {
//     auto stratumFilterMat =
//         std::make_unique<ComputeMaterial>(appContext, _logger, _makeShaderPath("stratumFilter"));
//     {
//       stratumFilterMat->addUniformBufferBundle(buffersHolder->getGlobalBufferBundle());
//       stratumFilterMat->addUniformBufferBundle(buffersHolder->getStratumFilterBufferBundle(i));
//       // input
//       stratumFilterMat->addStorageImage(imagesHolder->getPositionImage());
//       stratumFilterMat->addStorageImage(imagesHolder->getNormalImage());
//       stratumFilterMat->addStorageImage(imagesHolder->getDepthImage());
//       stratumFilterMat->addStorageImage(imagesHolder->getGradientImage());
//       stratumFilterMat->addStorageImage(imagesHolder->getRawImage());
//       stratumFilterMat->addStorageImage(imagesHolder->getStratumOffsetImage());
//       // pingpong
//       stratumFilterMat->addStorageImage(imagesHolder->getTemporalGradientNormalizationImagePing());
//       stratumFilterMat->addStorageImage(imagesHolder->getTemporalGradientNormalizationImagePong());
//     }
//     _stratumFilterModels.emplace_back(
//         std::make_unique<ComputeModel>(std::move(stratumFilterMat), 32, 32, 32));
//   }
// }
// void ModelsHolder::_createTemporalFilterModel(ImagesHolder *imagesHolder,
//                                               BuffersHolder *buffersHolder) {
//   auto temporalFilterMat =
//       std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath("temporalFilter"));
//   temporalFilterMat->addUniformBufferBundle(buffersHolder->getGlobalBufferBundle());
//   temporalFilterMat->addUniformBufferBundle(buffersHolder->getTemperalFilterBufferBundle());
//   // input
//   temporalFilterMat->addStorageImage(imagesHolder->getPositionImage());
//   temporalFilterMat->addStorageImage(imagesHolder->getRawImage());
//   temporalFilterMat->addStorageImage(imagesHolder->getDepthImage());
//   temporalFilterMat->addStorageImage(imagesHolder->getDepthImagePrev());
//   temporalFilterMat->addStorageImage(imagesHolder->getNormalImage());
//   temporalFilterMat->addStorageImage(imagesHolder->getNormalImagePrev());
//   temporalFilterMat->addStorageImage(imagesHolder->getGradientImage());
//   temporalFilterMat->addStorageImage(imagesHolder->getGradientImagePrev());
//   temporalFilterMat->addStorageImage(imagesHolder->getMeshHashImage());
//   temporalFilterMat->addStorageImage(imagesHolder->getMeshHashImagePrev());
//   temporalFilterMat->addStorageImage(imagesHolder->getLastFrameAccumImage());
//   temporalFilterMat->addStorageImage(imagesHolder->getVarianceHistImagePrev());
//   // output
//   temporalFilterMat->addStorageImage(imagesHolder->getATrousInputImage());
//   temporalFilterMat->addStorageImage(imagesHolder->getVarianceHistImage());
//   _temporalFilterModel =
//       std::make_unique<ComputeModel>(std::move(temporalFilterMat), 32, 32, 32);
// }

// void ModelsHolder::_createVarianceModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder)
// {

//   auto varianceMat = std::make_unique<ComputeMaterial>(_appContext, _logger,
//   _makeShaderPath("variance"));
//   {
//     varianceMat->addUniformBufferBundle(buffersHolder->getGlobalBufferBundle());
//     varianceMat->addUniformBufferBundle(buffersHolder->getVarianceBufferBundle());
//     // input
//     varianceMat->addStorageImage(imagesHolder->getATrousInputImage());
//     varianceMat->addStorageImage(imagesHolder->getNormalImage());
//     varianceMat->addStorageImage(imagesHolder->getDepthImage());
//     // output
//     varianceMat->addStorageImage(imagesHolder->getGradientImage());
//     varianceMat->addStorageImage(imagesHolder->getVarianceImage());
//     varianceMat->addStorageImage(imagesHolder->getVarianceHistImage());
//   }
//   _varianceModel = std::make_unique<ComputeModel>(std::move(varianceMat), 32, 32, 32);
// }

// void ModelsHolder::_createATrousModels(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder)
// {
//   _aTrousModels.clear();
//   for (int i = 0; i < kATrousSize; i++) {
//     auto aTrousMat = std::make_unique<ComputeMaterial>(_appContext, _logger,
//     makeShaderPath("aTrous"));
//     {
//       aTrousMat->addUniformBufferBundle(buffersHolder->getGlobalBufferBundle());
//       aTrousMat->addUniformBufferBundle(buffersHolder->getBlurFilterBufferBundle(i));

//       // readonly input
//       aTrousMat->addStorageImage(imagesHolder->getVec2BlueNoise());
//       aTrousMat->addStorageImage(imagesHolder->getWeightedCosineBlueNoise());

//       // input
//       aTrousMat->addStorageImage(imagesHolder->getATrousInputImage());
//       aTrousMat->addStorageImage(imagesHolder->getNormalImage());
//       aTrousMat->addStorageImage(imagesHolder->getDepthImage());
//       aTrousMat->addStorageImage(imagesHolder->getGradientImage());
//       aTrousMat->addStorageImage(imagesHolder->getVarianceImage());

//       // output
//       aTrousMat->addStorageImage(imagesHolder->getLastFrameAccumImage());
//       aTrousMat->addStorageImage(imagesHolder->getATrousOutputImage());
//     }
//     _aTrousModels.emplace_back(
//         std::make_unique<ComputeModel>(std::move(aTrousMat), 32, 32, 32));
//   }
// }

void ModelsHolder::_createPostProcessingModel(ImagesHolder *imagesHolder,
                                              BuffersHolder *buffersHolder, size_t framesInFlight) {
  std::string const shaderName = "postProcessing";
  auto postProcessingMat =
      std::make_unique<ComputeMaterial>(_appContext, _logger, _makeShaderPath(shaderName));
  postProcessingMat->addUniformBufferBundle(buffersHolder->getGlobalBufferBundle());
  postProcessingMat->addUniformBufferBundle(buffersHolder->getPostProcessingBufferBundle());
  // input
  postProcessingMat->addStorageImage(imagesHolder->getATrousOutputImage());
  postProcessingMat->addStorageImage(imagesHolder->getVarianceImage());
  postProcessingMat->addStorageImage(imagesHolder->getRawImage());
  postProcessingMat->addStorageImage(imagesHolder->getStratumOffsetImage());
  postProcessingMat->addStorageImage(imagesHolder->getVisibilityImage());
  postProcessingMat->addStorageImage(imagesHolder->getTemporalGradientNormalizationImagePong());
  postProcessingMat->addStorageImage(imagesHolder->getSeedImage());
  // output
  postProcessingMat->addStorageImage(imagesHolder->getTargetImage());
  _postProcessingModel = std::make_unique<ComputeModel>(
      std::move(postProcessingMat), framesInFlight, kShaderNameToWorkGroupSizes.at(shaderName));
}

void ModelsHolder::init(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder,
                        size_t framesInFlight) {
  // _createGradientProjectionModel(imagesHolder, buffersHolder);
  _createRtxModel(imagesHolder, buffersHolder, framesInFlight);
  // _createGradientModel(imagesHolder, buffersHolder);
  // _createStratumFilterModels(imagesHolder, buffersHolder);
  // _createTemporalFilterModel(imagesHolder, buffersHolder);
  // _createVarianceModel(imagesHolder, buffersHolder);
  // _createATrousModels(imagesHolder, buffersHolder);
  _createPostProcessingModel(imagesHolder, buffersHolder, framesInFlight);
}