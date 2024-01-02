#pragma once

#include "material/ComputeModel.hpp"

#include <memory>

class ImagesHolder;
class BuffersHolder;
class Logger;
class VulkanApplicationContext;
// should be initialized after ImagesHolder and BuffersHolder
class ModelsHolder {
public:
  ModelsHolder(VulkanApplicationContext *appContext, Logger *logger);

  // the image holder is required to be initialized first
  void init(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);

  // ComputeModel *getGradientProjectionModel() { return _gradientProjectionModel.get(); }
  ComputeModel *getRtxModel() { return _rtxModel.get(); }
  // ComputeModel *getGradientModel() { return _gradientModel.get(); }
  // ComputeModel *getStratumFilterModel(int index) { return _stratumFilterModels[index].get(); }
  // ComputeModel *getTemporalFilterModel() { return _temporalFilterModel.get(); }
  // ComputeModel *getVarianceModel() { return _varianceModel.get(); }
  // ComputeModel *getATrousModel(int index) { return _aTrousModels[index].get(); }
  ComputeModel *getPostProcessingModel() { return _postProcessingModel.get(); }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  // std::unique_ptr<ComputeModel> _gradientProjectionModel;
  std::unique_ptr<ComputeModel> _rtxModel;
  // std::unique_ptr<ComputeModel> _gradientModel;
  // std::vector<std::unique_ptr<ComputeModel>> _stratumFilterModels;
  // std::unique_ptr<ComputeModel> _temporalFilterModel;
  // std::unique_ptr<ComputeModel> _varianceModel;
  // std::vector<std::unique_ptr<ComputeModel>> _aTrousModels;
  std::unique_ptr<ComputeModel> _postProcessingModel;

  // void _createGradientProjectionModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);
  void _createRtxModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);
  // void _createGradientModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);
  // void _createStratumFilterModels(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);
  // void _createTemporalFilterModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);
  // void _createVarianceModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);
  // void _createATrousModels(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);
  void _createPostProcessingModel(ImagesHolder *imagesHolder, BuffersHolder *buffersHolder);
};