#include "BuffersHolder.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "memory/Buffer.hpp"
#include "memory/BufferBundle.hpp"

void BuffersHolder::init(SvoBuilder *svoScene, int stratumFilterSize, int aTrousSize,
                         size_t framesInFlight) {
  _createBufferBundles(stratumFilterSize, aTrousSize, framesInFlight);
  _createBuffers(svoScene);
}

// these buffers are modified by the CPU side every frame, and we have multiple frames in flight,
// so we need to create multiple copies of them, they are fairly small though
void BuffersHolder::_createBufferBundles(int stratumFilterSize, int aTrousSize,
                                         size_t framesInFlight) {
  _globalBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(GlobalUniformBufferObject), BufferType::kUniform);

  _gradientProjectionBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(GradientProjectionUniformBufferObject), BufferType::kUniform);

  for (int i = 0; i < stratumFilterSize; i++) {
    auto stratumFilterBufferBundle = std::make_unique<BufferBundle>(
        framesInFlight, sizeof(StratumFilterUniformBufferObject), BufferType::kUniform);
    _stratumFilterBufferBundle.emplace_back(std::move(stratumFilterBufferBundle));
  }

  _temperalFilterBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(TemporalFilterUniformBufferObject), BufferType::kUniform);

  _varianceBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(VarianceUniformBufferObject), BufferType::kUniform);

  for (int i = 0; i < aTrousSize; i++) {
    auto blurFilterBufferBundle = std::make_unique<BufferBundle>(
        framesInFlight, sizeof(BlurFilterUniformBufferObject), BufferType::kUniform);
    _blurFilterBufferBundles.emplace_back(std::move(blurFilterBufferBundle));
  }

  _postProcessingBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(PostProcessingUniformBufferObject), BufferType::kUniform);
}

// these buffers only need one copy! because they are not modified by CPU once uploaded to GPU
// side, and GPU cannot work on multiple frames at the same time
void BuffersHolder::_createBuffers(SvoBuilder *svoScene) {
  _voxelBuffer = std::make_unique<Buffer>(sizeof(uint32_t) * svoScene->getVoxelBuffer().size(),
                                          BufferType::kStorage);
  _voxelBuffer->fillData(svoScene->getVoxelBuffer().data());

  _paletteBuffer = std::make_unique<Buffer>(sizeof(uint32_t) * svoScene->getPaletteBuffer().size(),
                                            BufferType::kStorage);
  _paletteBuffer->fillData(svoScene->getPaletteBuffer().data());
}