#include "BuffersHolder.hpp"

#include "app-context/VulkanApplicationContext.hpp"

void BuffersHolder::init(GpuModel::TrisScene *rtScene, SvoScene *svoScene, int stratumFilterSize,
                         int aTrousSize, size_t framesInFlight) {
  _createBuffers(rtScene, svoScene);
  _createBufferBundles(stratumFilterSize, aTrousSize, framesInFlight);
}

// these buffers are modified by the CPU side every frame, and we have multiple frames in flight,
// so we need to create multiple copies of them, they are fairly small though
void BuffersHolder::_createBufferBundles(int stratumFilterSize, int aTrousSize,
                                         size_t framesInFlight) {
  _globalBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(GlobalUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  _gradientProjectionBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(GradientProjectionUniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  _rtxBufferBundle = std::make_unique<BufferBundle>(framesInFlight, sizeof(RtxUniformBufferObject),
                                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < stratumFilterSize; i++) {
    auto stratumFilterBufferBundle = std::make_unique<BufferBundle>(
        framesInFlight, sizeof(StratumFilterUniformBufferObject),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _stratumFilterBufferBundle.emplace_back(std::move(stratumFilterBufferBundle));
  }

  _temperalFilterBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(TemporalFilterUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  _varianceBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(VarianceUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < aTrousSize; i++) {
    auto blurFilterBufferBundle = std::make_unique<BufferBundle>(
        framesInFlight, sizeof(BlurFilterUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    _blurFilterBufferBundles.emplace_back(std::move(blurFilterBufferBundle));
  }

  _postProcessingBufferBundle = std::make_unique<BufferBundle>(
      framesInFlight, sizeof(PostProcessingUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);
}

// these buffers only need one copy! because they are not modified by CPU once uploaded to GPU
// side, and GPU cannot work on multiple frames at the same time
void BuffersHolder::_createBuffers(GpuModel::TrisScene *rtScene, SvoScene *svoScene) {
  _triangleBuffer = std::make_unique<Buffer>(
      sizeof(GpuModel::Triangle) * rtScene->triangles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->triangles.data());

  _materialBuffer = std::make_unique<Buffer>(
      sizeof(GpuModel::Material) * rtScene->materials.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->materials.data());

  _bvhBuffer = std::make_unique<Buffer>(sizeof(GpuModel::BvhNode) * rtScene->bvhNodes.size(),
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->bvhNodes.data());

  _lightsBuffer = std::make_unique<Buffer>(sizeof(GpuModel::Light) * rtScene->lights.size(),
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->lights.data());

  _voxelBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t) * svoScene->getVoxelBuffer().size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, svoScene->getVoxelBuffer().data());

  _paletteBuffer = std::make_unique<Buffer>(
      sizeof(uint32_t) * svoScene->getPaletteBuffer().size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, svoScene->getPaletteBuffer().data());
}