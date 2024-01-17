#include "FpsSink.hpp"

#include "MovingAvg.hpp"

#include <chrono>

size_t constexpr kMovingAvgSize              = 100;
double constexpr kBucketRefreshIntervalInSec = 0.2;

FpsSink::FpsSink() { _avg = std::make_unique<MovingAvg>(kMovingAvgSize); }

FpsSink::~FpsSink() = default;

void FpsSink::addRecord(double fps) {
  _updateMovingAvg(fps);
  _updateBucket(fps);
}

void FpsSink::_updateMovingAvg(double fps) { _avg->add(static_cast<float>(fps)); }

void FpsSink::_updateBucket(double fps) {
  static auto lastUpdateTime = std::chrono::steady_clock::now();

  auto now = std::chrono::steady_clock::now();
  auto delta =
      std::chrono::duration_cast<std::chrono::duration<double>>(now - lastUpdateTime).count();

  _fpsInTimeBucket.push_back(fps);

  if (delta > kBucketRefreshIntervalInSec) {
    // record the average
    for (auto fps : _fpsInTimeBucket) {
      _lastAvgInBucket += fps;
    }
    _lastAvgInBucket /= static_cast<float>(_fpsInTimeBucket.size());

    _fpsInTimeBucket.clear();
    lastUpdateTime = now;
  }
}

double FpsSink::getFilteredFps() const { return _avg->getAverage(); }

double FpsSink::getFpsInTimeBucket() const { return _lastAvgInBucket; }
