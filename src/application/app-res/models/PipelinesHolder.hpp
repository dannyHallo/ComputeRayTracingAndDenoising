#pragma once

#include <cassert>
#include <memory>
#include <vector>

class ImagesHolder;
class BuffersHolder;
class Logger;
class VulkanApplicationContext;
class DescriptorSetBundle;
class ComputePipeline;
class PipelinesHolder {
public:
  PipelinesHolder(VulkanApplicationContext *appContext, Logger *logger)
      : _appContext(appContext), _logger(logger) {}

  ~PipelinesHolder();

  // disable copy and move
  PipelinesHolder(const PipelinesHolder &)            = delete;
  PipelinesHolder &operator=(const PipelinesHolder &) = delete;
  PipelinesHolder(PipelinesHolder &&)                 = delete;
  PipelinesHolder &operator=(PipelinesHolder &&)      = delete;

  // the image holder is required to be initialized first
  void init(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder, int stratumFilterSize,
            int aTrousSize, size_t framesInFlight);

  ComputePipeline *getGradientProjectionPipeline() { return _gradientProjectionPipeline.get(); }

  ComputePipeline *getRtxPipeline() { return _rtxPipeline.get(); }
  ComputePipeline *getSvoPipeline() { return _svoPipeline.get(); }

  ComputePipeline *getGradientPipeline() { return _gradientPipeline.get(); }
  ComputePipeline *getStratumFilterPipeline(int index) {
    return _stratumFilterPipelines[index].get();
  }

  ComputePipeline *getTemporalFilterPipeline() { return _temporalFilterPipeline.get(); }
  ComputePipeline *getVariancePipeline() { return _variancePipeline.get(); }

  ComputePipeline *getATrousPipeline(int index) { return _aTrousPipelines[index].get(); }

  ComputePipeline *getPostProcessingPipeline() { return _postProcessingPipeline.get(); }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  ImagesHolder *_imagesHolder   = nullptr;
  BuffersHolder *_buffersHolder = nullptr;
  size_t _framesInFlight        = 0;

  int _stratumFilterSize = 0;
  int _aTrousSize        = 0;

  std::unique_ptr<DescriptorSetBundle> _dsb;

  std::unique_ptr<ComputePipeline> _gradientProjectionPipeline;
  std::unique_ptr<ComputePipeline> _rtxPipeline;
  std::unique_ptr<ComputePipeline> _svoPipeline;
  std::unique_ptr<ComputePipeline> _gradientPipeline;
  std::vector<std::unique_ptr<ComputePipeline>> _stratumFilterPipelines;
  std::unique_ptr<ComputePipeline> _temporalFilterPipeline;
  std::unique_ptr<ComputePipeline> _variancePipeline;
  std::vector<std::unique_ptr<ComputePipeline>> _aTrousPipelines;
  std::unique_ptr<ComputePipeline> _postProcessingPipeline;

  void _createDescriptorSetBundle();
  void _createComputePipelines();
};