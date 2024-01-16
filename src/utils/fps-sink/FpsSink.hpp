#pragma once

#include <memory>
#include <vector>

class MovingAvg;
class FpsSink {
public:
  FpsSink();
  ~FpsSink();

  // delete copy and move
  FpsSink(const FpsSink &)            = delete;
  FpsSink &operator=(const FpsSink &) = delete;
  FpsSink(FpsSink &&)                 = delete;
  FpsSink &operator=(FpsSink &&)      = delete;

  void addRecord(double fps);

  // get fps with frequent updates, for plot use
  [[nodiscard]] double getFilteredFps() const;

  // get fps with less frequent updates, digit is human readable
  [[nodiscard]] double getFpsInTimeBucket() const;

private:
  std::unique_ptr<MovingAvg> _avg;
  std::vector<double> _fpsInTimeBucket{};
  double _lastAvgInBucket = 0.0;

  void _updateMovingAvg(double fps);
  void _updateBucket(double fps);
};