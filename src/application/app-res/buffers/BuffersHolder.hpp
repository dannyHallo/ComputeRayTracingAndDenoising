#pragma once

#include "svo-ray-tracing/SvoBuilder.hpp"
#include "utils/incl/Glm.hpp"

#include <memory>

constexpr int kGpuVecPackingSize = 4;

class Buffer;
class BufferBundle;
struct GlobalUniformBufferObject {
  alignas(sizeof(glm::vec3::x) * kGpuVecPackingSize) glm::vec3 camPosition;
  alignas(sizeof(glm::vec3::x) * kGpuVecPackingSize) glm::vec3 camFront;
  alignas(sizeof(glm::vec3::x) * kGpuVecPackingSize) glm::vec3 camUp;
  alignas(sizeof(glm::vec3::x) * kGpuVecPackingSize) glm::vec3 camRight;
  uint32_t swapchainWidth;
  uint32_t swapchainHeight;
  float vfov;
  uint32_t currentSample;
  float time;
};

struct GradientProjectionUniformBufferObject {
  int bypassGradientProjection;
  alignas(sizeof(glm::vec3::x) * kGpuVecPackingSize) glm::mat4 thisMvpe;
};

struct StratumFilterUniformBufferObject {
  int i;
  int bypassStratumFiltering;
};

struct TemporalFilterUniformBufferObject {
  int bypassTemporalFiltering;
  int useNormalTest;
  float normalThrehold;
  float blendingAlpha;
  alignas(sizeof(glm::vec3::x) * kGpuVecPackingSize) glm::mat4 lastMvpe;
};
struct VarianceUniformBufferObject {
  int bypassVarianceEstimation;
  int skipStoppingFunctions;
  int useTemporalVariance;
  int kernelSize;
  float phiGaussian;
  float phiDepth;
};

struct BlurFilterUniformBufferObject {
  int bypassBluring;
  int i;
  int iCap;
  int useVarianceGuidedFiltering;
  int useGradientInDepth;
  float phiLuminance;
  float phiDepth;
  float phiNormal;
  int ignoreLuminanceAtFirstIteration;
  int changingLuminancePhi;
  int useJittering;
};

struct PostProcessingUniformBufferObject {
  uint32_t displayType;
};

class VulkanApplicationContext;
class BuffersHolder {
public:
  BuffersHolder() = default;

  void init(SvoBuilder *svoScene, int stratumFilterSize, int aTrousSize, size_t framesInFlight);

  // buffer bundles getter
  BufferBundle *getGlobalBufferBundle() { return _globalBufferBundle.get(); }
  BufferBundle *getGradientProjectionBufferBundle() {
    return _gradientProjectionBufferBundle.get();
  }
  BufferBundle *getTemperalFilterBufferBundle() { return _temperalFilterBufferBundle.get(); }
  BufferBundle *getStratumFilterBufferBundle(int index) {
    return _stratumFilterBufferBundle[index].get();
  }
  BufferBundle *getVarianceBufferBundle() { return _varianceBufferBundle.get(); }
  BufferBundle *getBlurFilterBufferBundle(int index) {
    return _blurFilterBufferBundles[index].get();
  }
  BufferBundle *getPostProcessingBufferBundle() { return _postProcessingBufferBundle.get(); }

  Buffer *getVoxelBuffer() { return _voxelBuffer.get(); }
  Buffer *getPaletteBuffer() { return _paletteBuffer.get(); }

private:
  // https://www.reddit.com/r/vulkan/comments/10io2l8/is_framesinflight_fif_method_really_worth_it/
  // You seem to be saying you keep #FIF copies of all resources. Only resources that are actively
  // accessed by both the CPU and GPU need to because the GPU will not work on multiple frames at
  // the same time. You don't need to multiply textures or buffers in device memory. This means that
  // the memory increase should be small and solves your problem with uploading data.

  // buffers that are updated by CPU and sent to GPU every frame
  std::unique_ptr<BufferBundle> _globalBufferBundle;
  std::unique_ptr<BufferBundle> _gradientProjectionBufferBundle;
  std::unique_ptr<BufferBundle> _temperalFilterBufferBundle;
  std::vector<std::unique_ptr<BufferBundle>> _stratumFilterBufferBundle;
  std::unique_ptr<BufferBundle> _varianceBufferBundle;
  std::vector<std::unique_ptr<BufferBundle>> _blurFilterBufferBundles;
  std::unique_ptr<BufferBundle> _postProcessingBufferBundle;

  // buffers that are updated by CPU and sent to GPU only once
  std::unique_ptr<Buffer> _voxelBuffer;
  std::unique_ptr<Buffer> _paletteBuffer;

  void _createBufferBundles(int stratumFilterSize, int aTrousSize, size_t framesInFlight);
  void _createBuffers(SvoBuilder *svoScene);
};