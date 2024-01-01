#include "BuffersHolder.hpp"

#include "app-context/VulkanApplicationContext.hpp"

static constexpr int kATrousSize = 5;

BuffersHolder::BuffersHolder(VulkanApplicationContext *appContext) : _appContext(appContext) {}

void BuffersHolder::init(GpuModel::RtScene *rtScene) {
  // equals to descriptor sets size
  auto swapchainSize = static_cast<uint32_t>(_appContext->getSwapchainSize());
  // uniform buffers are faster to fill compared to storage buffers, but they
  // are restricted in their size Buffer bundle is an array of buffers, one per
  // each swapchain image/descriptor set.
  _globalBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GlobalUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  _gradientProjectionBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GradientProjectionUniformBufferObject),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  _rtxBufferBundle = std::make_unique<BufferBundle>(swapchainSize, sizeof(RtxUniformBufferObject),
                                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < 6; i++) {
    auto stratumFilterBufferBundle = std::make_unique<BufferBundle>(
        swapchainSize, sizeof(StratumFilterUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    _stratumFilterBufferBundle.emplace_back(std::move(stratumFilterBufferBundle));
  }

  _temperalFilterBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(TemporalFilterUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  _varianceBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(VarianceUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < kATrousSize; i++) {
    auto blurFilterBufferBundle = std::make_unique<BufferBundle>(
        swapchainSize, sizeof(BlurFilterUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    _blurFilterBufferBundles.emplace_back(std::move(blurFilterBufferBundle));
  }

  _postProcessingBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(PostProcessingUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  _triangleBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Triangle) * rtScene->triangles.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->triangles.data());

  _materialBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Material) * rtScene->materials.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->materials.data());

  _bvhBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::BvhNode) * rtScene->bvhNodes.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->bvhNodes.data());

  _lightsBufferBundle = std::make_unique<BufferBundle>(
      swapchainSize, sizeof(GpuModel::Light) * rtScene->lights.size(),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, rtScene->lights.data());
}