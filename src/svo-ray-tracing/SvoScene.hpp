#pragma once

#include "svo-ray-tracing/im-data/ImData.hpp"
#include "svo-ray-tracing/octree/BaseLevelBuilder.hpp"
#include "svo-ray-tracing/octree/UpperLevelBuilder.hpp"

#include <memory>
#include <vector>

class Logger;
class SvoScene {
public:
  SvoScene(Logger *logger);

  [[nodiscard]] const std::vector<uint32_t> &getBuffer() const { return _buffer; }

private:
  Logger *_logger;
  std::vector<uint32_t> _buffer;
  std::vector<std::unique_ptr<ImageData>> _imageDatas;

  void _run();

  void _buildImageDatas();
  void _printImageDatas();
  void _createBuffer();
  void _printBuffer();
};