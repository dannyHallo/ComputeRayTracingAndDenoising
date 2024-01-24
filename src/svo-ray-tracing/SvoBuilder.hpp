#pragma once

#include <array>
#include <memory>
#include <vector>

class Logger;
struct VoxData;

class SvoBuilder {
public:
  SvoBuilder(Logger *logger);
  ~SvoBuilder();

  // disable copy and move
  SvoBuilder(SvoBuilder const &)            = delete;
  SvoBuilder(SvoBuilder &&)                 = delete;
  SvoBuilder &operator=(SvoBuilder const &) = delete;
  SvoBuilder &operator=(SvoBuilder &&)      = delete;

  [[nodiscard]] const std::vector<uint32_t> &getVoxelBuffer() const { return _voxelBuffer; }
  [[nodiscard]] const std::array<uint32_t, 256> &getPaletteBuffer() const { // NOLINT
    return _paletteBuffer;
  }

private:
  Logger *_logger;

  std::vector<uint32_t> _voxelBuffer;
  std::array<uint32_t, 256> _paletteBuffer{}; // NOLINT

  std::unique_ptr<VoxData> _voxData;

  void _fetchRawVoxelData(size_t index);
  void _createVoxelBuffer();
};