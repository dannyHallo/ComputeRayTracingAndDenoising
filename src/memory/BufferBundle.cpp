#include "BufferBundle.hpp"

#include <cassert>

Buffer *BufferBundle::getBuffer(size_t index) {
  assert((index >= 0 && index < _buffers.size()) && "BufferBundle::getBuffer: index out of range");
  return _buffers[index].get();
}

Buffer *BufferBundle::operator[](size_t index) { return getBuffer(index); }

void BufferBundle::fillData(const void *data) {
  for (auto &buffer : _buffers) {
    buffer->fillData(data);
  }
}