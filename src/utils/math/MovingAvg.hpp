#pragma once

#include <vector>

class MovingAvg {
public:
  MovingAvg(size_t n) : capacity(n) { buffer.resize(capacity, 0.0); }

  void add(float fps);

  [[nodiscard]] float getAverage() const {
    return (count == 0) ? 0.0F : sum / static_cast<float>(count);
  }

private:
  std::vector<float> buffer;
  size_t capacity;
  size_t count = 0;
  size_t index = 0;
  float sum    = 0;
};
