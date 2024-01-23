#include "PipelinesHolder.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "application/app-res/buffers/BuffersHolder.hpp"
#include "application/app-res/images/ImagesHolder.hpp"
#include "material/ComputePipeline.hpp"
#include "material/DescriptorSetBundle.hpp"

#include "utils/config/RootDir.h"
#include "utils/logger/Logger.hpp"

#include <map>

PipelinesHolder::~PipelinesHolder() = default;

void PipelinesHolder::_createDescriptorSetBundle() {
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

void PipelinesHolder::_createComputePipelines() {
  {
    std::vector<uint32_t> shaderCode{
#include "svoTracing.spv"
    };
    _svoPipeline = std::make_unique<ComputePipeline>(_appContext, _logger, std::move(shaderCode),
                                                     WorkGroupSize{8, 8, 1}, _dsb.get());
    _svoPipeline->create();
  }
  {
    std::vector<uint32_t> shaderCode{
#include "postProcessing.spv"
    };
    _postProcessingPipeline = std::make_unique<ComputePipeline>(
        _appContext, _logger, std::move(shaderCode), WorkGroupSize{8, 8, 1}, _dsb.get());
    _postProcessingPipeline->create();
  }
}

void PipelinesHolder::init(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder,
                           int stratumFilterSize, int aTrousSize, size_t framesInFlight) {
  _imagesHolder  = imagesHolder;
  _buffersHolder = buffersHolder;

  _stratumFilterSize = stratumFilterSize;
  _aTrousSize        = aTrousSize;
  _framesInFlight    = framesInFlight;

  _createDescriptorSetBundle();
  _createComputePipelines();
}