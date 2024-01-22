#pragma once

#include <cassert>
#include <memory>
#include <vector>

class ComputeModel;
class ImagesHolder;
class BuffersHolder;
class Logger;
class VulkanApplicationContext;
class DescriptorSetBundle;
class ModelsHolder {
public:
  ModelsHolder(VulkanApplicationContext *appContext, Logger *logger)
      : _appContext(appContext), _logger(logger) {}

  ~ModelsHolder();

  // disable copy and move
  ModelsHolder(const ModelsHolder &)            = delete;
  ModelsHolder &operator=(const ModelsHolder &) = delete;
  ModelsHolder(ModelsHolder &&)                 = delete;
  ModelsHolder &operator=(ModelsHolder &&)      = delete;

  // the image holder is required to be initialized first
  void init(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder, int stratumFilterSize,
            int aTrousSize, size_t framesInFlight);

  ComputeModel *getGradientProjectionModel() { return _gradientProjectionModel.get(); }

  ComputeModel *getRtxModel() { return _rtxModel.get(); }
  ComputeModel *getSvoModel() { return _svoModel.get(); }

  ComputeModel *getGradientModel() { return _gradientModel.get(); }
  ComputeModel *getStratumFilterModel(int index) { return _stratumFilterModels[index].get(); }

  ComputeModel *getTemporalFilterModel() { return _temporalFilterModel.get(); }
  ComputeModel *getVarianceModel() { return _varianceModel.get(); }

  ComputeModel *getATrousModel(int index) { return _aTrousModels[index].get(); }

  ComputeModel *getPostProcessingModel() { return _postProcessingModel.get(); }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  ImagesHolder *_imagesHolder   = nullptr;
  BuffersHolder *_buffersHolder = nullptr;
  size_t _framesInFlight        = 0;

  int _stratumFilterSize = 0;
  int _aTrousSize        = 0;

  std::unique_ptr<DescriptorSetBundle> _dsb;
  std::unique_ptr<ComputeModel> _gradientProjectionModel;
  std::unique_ptr<ComputeModel> _rtxModel;
  std::unique_ptr<ComputeModel> _svoModel;
  std::unique_ptr<ComputeModel> _gradientModel;
  std::vector<std::unique_ptr<ComputeModel>> _stratumFilterModels;
  std::unique_ptr<ComputeModel> _temporalFilterModel;
  std::unique_ptr<ComputeModel> _varianceModel;
  std::vector<std::unique_ptr<ComputeModel>> _aTrousModels;
  std::unique_ptr<ComputeModel> _postProcessingModel;

  void _createDescriptorSetBundle();

  void _createGradientProjectionModel();
  void _createRtxModel();
  void _createSvoModel();
  void _createGradientModel();
  void _createStratumFilterModels();
  void _createTemporalFilterModel();
  void _createVarianceModel();
  void _createATrousModels();
  void _createPostProcessingModel();
};