#pragma once

#include "material/ComputeModel.hpp"

#include <cassert>
#include <memory>

class ImagesHolder;
class BuffersHolder;
class Logger;
class VulkanApplicationContext;
// should be initialized after ImagesHolder and BuffersHolder
class ModelsHolder {
public:
  ModelsHolder(VulkanApplicationContext *appContext, Logger *logger)
      : _appContext(appContext), _logger(logger) {}

  // the image holder is required to be initialized first
  void init(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder, int stratumFilterSize,
            int aTrousSize, size_t framesInFlight);

  ComputeModel *getGradientProjectionModel() {
    assert(_gradientProjectionModel != nullptr && "Gradient projection model is not initialized");
    return _gradientProjectionModel.get();
  }

  ComputeModel *getRtxModel() {
    assert(_rtxModel != nullptr && "RTX model is not initialized");
    return _rtxModel.get();
  }
  ComputeModel *getSvoModel() {
    assert(_svoModel != nullptr && "SVO model is not initialized");
    return _svoModel.get();
  }

  ComputeModel *getGradientModel() {
    assert(_gradientModel != nullptr && "Gradient model is not initialized");
    return _gradientModel.get();
  }
  ComputeModel *getStratumFilterModel(int index) {
    assert(_stratumFilterModels[index] != nullptr && "Stratum filter model is not initialized");
    return _stratumFilterModels[index].get();
  }

  ComputeModel *getTemporalFilterModel() {
    assert(_temporalFilterModel != nullptr && "Temporal filter model is not initialized");
    return _temporalFilterModel.get();
  }
  ComputeModel *getVarianceModel() {
    assert(_varianceModel != nullptr && "Variance model is not initialized");
    return _varianceModel.get();
  }

  ComputeModel *getATrousModel(int index) {
    assert(_aTrousModels[index] != nullptr && "A-Trous model is not initialized");
    return _aTrousModels[index].get();
  }

  ComputeModel *getPostProcessingModel() {
    assert(_postProcessingModel != nullptr && "Post processing model is not initialized");
    return _postProcessingModel.get();
  }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  ImagesHolder *_imagesHolder   = nullptr;
  BuffersHolder *_buffersHolder = nullptr;
  size_t _framesInFlight        = 0;

  int _stratumFilterSize = 0;
  int _aTrousSize        = 0;

  std::unique_ptr<ComputeModel> _gradientProjectionModel;
  std::unique_ptr<ComputeModel> _rtxModel;
  std::unique_ptr<ComputeModel> _svoModel;
  std::unique_ptr<ComputeModel> _gradientModel;
  std::vector<std::unique_ptr<ComputeModel>> _stratumFilterModels;
  std::unique_ptr<ComputeModel> _temporalFilterModel;
  std::unique_ptr<ComputeModel> _varianceModel;
  std::vector<std::unique_ptr<ComputeModel>> _aTrousModels;
  std::unique_ptr<ComputeModel> _postProcessingModel;

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