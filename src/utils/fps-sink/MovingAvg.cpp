#include "MovingAvg.hpp"

void MovingAvg::add(float fps) {
  sum -= buffer[index];
  buffer[index] = fps;
  sum += fps;

  // Update index and count
  index = (index + 1) % capacity;
  if (count < capacity) {
    count++;
  }
}
