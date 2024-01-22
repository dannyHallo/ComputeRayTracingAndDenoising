#include "ModelsHolder.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "application/app-res/buffers/BuffersHolder.hpp"
#include "application/app-res/images/ImagesHolder.hpp"
#include "material/ComputeModel.hpp"
#include "material/DescriptorSetBundle.hpp"

#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <map>

ModelsHolder::~ModelsHolder() = default;

void ModelsHolder::_createDescriptorSetBundle() {
  _dsb = std::make_unique<DescriptorSetBundle>(_appContext, _logger, _framesInFlight,
                                               VK_SHADER_STAGE_COMPUTE_BIT);

  _dsb->bindUniformBufferBundle(0, _buffersHolder->getGlobalBufferBundle());
  _dsb->bindUniformBufferBundle(1, _buffersHolder->getPostProcessingBufferBundle());

  _dsb->bindStorageImage(2, _imagesHolder->getRawImage());
  _dsb->bindStorageImage(3, _imagesHolder->getRawImage());
  _dsb->bindStorageImage(4, _imagesHolder->getTargetImage());

  _dsb->bindStorageBuffer(5, _buffersHolder->getVoxelBuffer());
  _dsb->bindStorageBuffer(6, _buffersHolder->getPaletteBuffer());

  _dsb->create();
}

// void ModelsHolder::_createGradientProjectionModel() {
//   // mat creation
//   std::vector<uint32_t> shaderCode{
//       // #include "shaders/generated/gradientProjection.spv"
//   };
//   auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode));

//   // ubo
//   mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
//   mat->addUniformBufferBundle(_buffersHolder->getGradientProjectionBufferBundle());

//   // image read
//   mat->addStorageImage(_imagesHolder->getVec2BlueNoise());
//   mat->addStorageImage(_imagesHolder->getWeightedCosineBlueNoise());
//   mat->addStorageImage(_imagesHolder->getRawImage());
//   mat->addStorageImage(_imagesHolder->getPositionImage());
//   mat->addStorageImage(_imagesHolder->getSeedImage());
//   // image write
//   mat->addStorageImage(_imagesHolder->getStratumOffsetImage());
//   // (atomic) readwrite
//   mat->addStorageImage(_imagesHolder->getPerStratumLockingImage());
//   // image write
//   mat->addStorageImage(_imagesHolder->getVisibilityImage());
//   mat->addStorageImage(_imagesHolder->getSeedVisibilityImage());

//   // model creation
//   _gradientProjectionModel =
//       std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1});
// }

// void ModelsHolder::_createRtxModel() {
//   // mat creation
//   std::vector<uint32_t> shaderCode{
//       // #include "shaders/generated/trisTracing.spv"
//   };
//   auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode));

//   // ubo
//   mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
//   mat->addUniformBufferBundle(_buffersHolder->getRtxBufferBundle());

//   // image read
//   mat->addStorageImage(_imagesHolder->getVec2BlueNoise());
//   mat->addStorageImage(_imagesHolder->getWeightedCosineBlueNoise());
//   mat->addStorageImage(_imagesHolder->getStratumOffsetImage());
//   mat->addStorageImage(_imagesHolder->getVisibilityImage());
//   mat->addStorageImage(_imagesHolder->getSeedVisibilityImage());

//   // image write
//   mat->addStorageImage(_imagesHolder->getPositionImage());
//   mat->addStorageImage(_imagesHolder->getNormalImage());
//   mat->addStorageImage(_imagesHolder->getDepthImage());
//   mat->addStorageImage(_imagesHolder->getMeshHashImage());
//   mat->addStorageImage(_imagesHolder->getRawImage());
//   mat->addStorageImage(_imagesHolder->getSeedImage());
//   mat->addStorageImage(_imagesHolder->getTemporalGradientNormalizationImagePing());

//   // buffers
//   mat->addStorageBufferBundle(_buffersHolder->getTriangleBufferBundle());
//   mat->addStorageBufferBundle(_buffersHolder->getMaterialBufferBundle());
//   mat->addStorageBufferBundle(_buffersHolder->getBvhBufferBundle());
//   mat->addStorageBufferBundle(_buffersHolder->getLightsBufferBundle());

//   // model creation
//   _rtxModel =
//       std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1});
// }

void ModelsHolder::_createSvoModel() {
  // mat creation
  std::vector<uint32_t> shaderCode{
#include "svoTracing.spv"
  };
  auto mat =
      std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode), _dsb.get());
  // model creation
  _svoModel =
      std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1});
}

// void ModelsHolder::_createGradientModel() {
//   // mat creation
//   std::vector<uint32_t> shaderCode{
//       // #include "shaders/generated/screenSpaceGradient.spv"
//   };
//   auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode));

//   // ubo
//   mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());

//   // image read
//   mat->addStorageImage(_imagesHolder->getDepthImage());

//   // image write
//   mat->addStorageImage(_imagesHolder->getGradientImage());

//   // model creation
//   _gradientModel =
//       std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1});
// }

// void ModelsHolder::_createStratumFilterModels() {
//   _stratumFilterModels.clear();
//   for (int i = 0; i < _stratumFilterSize; i++) {
//     // mat creation
//     std::vector<uint32_t> shaderCode{
//         // #include "shaders/generated/stratumFilter.spv"
//     };
//     auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode));

//     // ubo
//     mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
//     mat->addUniformBufferBundle(_buffersHolder->getStratumFilterBufferBundle(i));

//     // image read
//     mat->addStorageImage(_imagesHolder->getPositionImage());
//     mat->addStorageImage(_imagesHolder->getNormalImage());
//     mat->addStorageImage(_imagesHolder->getDepthImage());
//     mat->addStorageImage(_imagesHolder->getGradientImage());
//     mat->addStorageImage(_imagesHolder->getRawImage());
//     mat->addStorageImage(_imagesHolder->getStratumOffsetImage());

//     // pingpong
//     mat->addStorageImage(_imagesHolder->getTemporalGradientNormalizationImagePing());
//     mat->addStorageImage(_imagesHolder->getTemporalGradientNormalizationImagePong());

//     // model creation
//     _stratumFilterModels.emplace_back(
//         std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1}));
//   }
// }

// void ModelsHolder::_createTemporalFilterModel() {
//   // mat creation
//   std::vector<uint32_t> shaderCode{
//       // #include "shaders/generated/temporalFilter.spv"
//   };
//   auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode));

//   // ubo
//   mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
//   mat->addUniformBufferBundle(_buffersHolder->getTemperalFilterBufferBundle());

//   // image read
//   mat->addStorageImage(_imagesHolder->getPositionImage());
//   mat->addStorageImage(_imagesHolder->getRawImage());
//   mat->addStorageImage(_imagesHolder->getDepthImage());
//   mat->addStorageImage(_imagesHolder->getDepthImagePrev());
//   mat->addStorageImage(_imagesHolder->getNormalImage());
//   mat->addStorageImage(_imagesHolder->getNormalImagePrev());
//   mat->addStorageImage(_imagesHolder->getGradientImage());
//   mat->addStorageImage(_imagesHolder->getGradientImagePrev());
//   mat->addStorageImage(_imagesHolder->getMeshHashImage());
//   mat->addStorageImage(_imagesHolder->getMeshHashImagePrev());
//   mat->addStorageImage(_imagesHolder->getLastFrameAccumImage());
//   mat->addStorageImage(_imagesHolder->getVarianceHistImagePrev());

//   // image write
//   mat->addStorageImage(_imagesHolder->getATrousInputImage());
//   mat->addStorageImage(_imagesHolder->getVarianceHistImage());

//   // model creation
//   _temporalFilterModel =
//       std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1});
// }

// void ModelsHolder::_createVarianceModel() {
//   // mat creation
//   std::vector<uint32_t> shaderCode{
//       // #include "shaders/generated/variance.spv"
//   };
//   auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode));

//   // ubo
//   mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
//   mat->addUniformBufferBundle(_buffersHolder->getVarianceBufferBundle());

//   // image read
//   mat->addStorageImage(_imagesHolder->getATrousInputImage());
//   mat->addStorageImage(_imagesHolder->getNormalImage());
//   mat->addStorageImage(_imagesHolder->getDepthImage());

//   // image write
//   mat->addStorageImage(_imagesHolder->getGradientImage());
//   mat->addStorageImage(_imagesHolder->getVarianceImage());
//   mat->addStorageImage(_imagesHolder->getVarianceHistImage());

//   // model creation
//   _varianceModel =
//       std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1});
// }

// void ModelsHolder::_createATrousModels() {
//   _aTrousModels.clear();
//   for (int i = 0; i < _aTrousSize; i++) {
//     // mat creation
//     std::vector<uint32_t> shaderCode{
//         // #include "shaders/generated/aTrous.spv"
//     };
//     auto mat = std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode));

//     // ubo
//     mat->addUniformBufferBundle(_buffersHolder->getGlobalBufferBundle());
//     mat->addUniformBufferBundle(_buffersHolder->getBlurFilterBufferBundle(i));

//     // image read
//     mat->addStorageImage(_imagesHolder->getVec2BlueNoise());
//     mat->addStorageImage(_imagesHolder->getWeightedCosineBlueNoise());
//     mat->addStorageImage(_imagesHolder->getATrousInputImage());
//     mat->addStorageImage(_imagesHolder->getNormalImage());
//     mat->addStorageImage(_imagesHolder->getDepthImage());
//     mat->addStorageImage(_imagesHolder->getGradientImage());
//     mat->addStorageImage(_imagesHolder->getVarianceImage());

//     // image write
//     mat->addStorageImage(_imagesHolder->getLastFrameAccumImage());
//     mat->addStorageImage(_imagesHolder->getATrousOutputImage());

//     // model creation
//     _aTrousModels.emplace_back(
//         std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1}));
//   }
// }

void ModelsHolder::_createPostProcessingModel() {
  // mat creation
  std::vector<uint32_t> shaderCode{
#include "postProcessing.spv"
  };
  auto mat =
      std::make_unique<ComputeMaterial>(_appContext, _logger, std::move(shaderCode), _dsb.get());

  // model creation
  _postProcessingModel =
      std::make_unique<ComputeModel>(std::move(mat), _framesInFlight, WorkGroupSize{8, 8, 1});
}

void ModelsHolder::init(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder,
                        int stratumFilterSize, int aTrousSize, size_t framesInFlight) {
  _imagesHolder  = imagesHolder;
  _buffersHolder = buffersHolder;

  _stratumFilterSize = stratumFilterSize;
  _aTrousSize        = aTrousSize;
  _framesInFlight    = framesInFlight;

  _createDescriptorSetBundle();

  _createSvoModel();
  _createPostProcessingModel();
}