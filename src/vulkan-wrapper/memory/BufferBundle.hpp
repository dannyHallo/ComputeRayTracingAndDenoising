#pragma once

#include "Buffer.hpp"

#include <memory>
#include <vector>

class BufferBundle {
public:
  BufferBundle(VulkanApplicationContext *appContext, size_t bundleSize, VkDeviceSize perBufferSize,
               VkBufferUsageFlags bufferUsageFlags, MemoryStyle memoryAccessingStyle) {
    _buffers.reserve(bundleSize);
    for (size_t i = 0; i < bundleSize; i++) {
      _buffers.emplace_back(std::make_unique<Buffer>(appContext, perBufferSize, bufferUsageFlags,
                                                     memoryAccessingStyle));
    }
  }

  ~BufferBundle() { _buffers.clear(); }

  // delete copy and move
  BufferBundle(const BufferBundle &)            = delete;
  BufferBundle &operator=(const BufferBundle &) = delete;
  BufferBundle(BufferBundle &&)                 = delete;
  BufferBundle &operator=(BufferBundle &&)      = delete;

  [[nodiscard]] size_t getBundleSize() const { return _buffers.size(); }

  Buffer *getBuffer(size_t index);
  Buffer *operator[](size_t index);

  // fill every buffer in bundle with the same data, if left as nullptr, every
  // buffer will be zero-initialized
  void fillData(const void *data = nullptr);

private:
  std::vector<std::unique_ptr<Buffer>> _buffers; // series of buffers
};
