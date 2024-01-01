#pragma once

#include "memory/Buffer.hpp"
#include "ray-tracing/RtScene.hpp"
#include "utils/incl/Glm.hpp"

#include <memory>

struct GlobalUniformBufferObject {
  alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camPosition;
  alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camFront;
  alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camUp;
  alignas(sizeof(glm::vec3::x) * 4) glm::vec3 camRight;
  uint32_t swapchainWidth;
  uint32_t swapchainHeight;
  float vfov;
  uint32_t currentSample;
  float time;
};

struct GradientProjectionUniformBufferObject {
  int bypassGradientProjection;
  alignas(sizeof(glm::vec3::x) * 4) glm::mat4 thisMvpe;
};

struct RtxUniformBufferObject {
  uint32_t numTriangles;
  uint32_t numLights;
  int movingLightSource;
  uint32_t outputType;
  float offsetX;
  float offsetY;
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
  alignas(sizeof(glm::vec3::x) * 4) glm::mat4 lastMvpe;
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
  BuffersHolder(VulkanApplicationContext *appContext);

  void init(GpuModel::RtScene *rtScene);

  // buffer bundles getter
  BufferBundle *getGlobalBufferBundle() { return _globalBufferBundle.get(); }
  BufferBundle *getGradientProjectionBufferBundle() {
    return _gradientProjectionBufferBundle.get();
  }
  BufferBundle *getRtxBufferBundle() { return _rtxBufferBundle.get(); }
  BufferBundle *getTemperalFilterBufferBundle() { return _temperalFilterBufferBundle.get(); }
  BufferBundle *getStratumFilterBufferBundle(int index) {
    return _stratumFilterBufferBundle[index].get();
  }
  BufferBundle *getVarianceBufferBundle() { return _varianceBufferBundle.get(); }
  BufferBundle *getBlurFilterBufferBundle(int index) {
    return _blurFilterBufferBundles[index].get();
  }
  BufferBundle *getPostProcessingBufferBundle() { return _postProcessingBufferBundle.get(); }

  BufferBundle *getTriangleBufferBundle() { return _triangleBufferBundle.get(); }
  BufferBundle *getMaterialBufferBundle() { return _materialBufferBundle.get(); }
  BufferBundle *getBvhBufferBundle() { return _bvhBufferBundle.get(); }
  BufferBundle *getLightsBufferBundle() { return _lightsBufferBundle.get(); }

  // buffers getter
  std::shared_ptr<Buffer> getGlobalBuffer(int index) {
    return _globalBufferBundle->getBuffer(index);
  }
  std::shared_ptr<Buffer> getGradientProjectionBuffer(int index) {
    return _gradientProjectionBufferBundle->getBuffer(index);
  }
  std::shared_ptr<Buffer> getRtxBuffer(int index) { return _rtxBufferBundle->getBuffer(index); }
  std::shared_ptr<Buffer> getTemperalFilterBuffer(int index) {
    return _temperalFilterBufferBundle->getBuffer(index);
  }
  std::shared_ptr<Buffer> getStratumFilterBuffer(int index, int index2) {
    return _stratumFilterBufferBundle[index]->getBuffer(index2);
  }
  std::shared_ptr<Buffer> getVarianceBuffer(int index) {
    return _varianceBufferBundle->getBuffer(index);
  }
  std::shared_ptr<Buffer> getBlurFilterBuffer(int index, int index2) {
    return _blurFilterBufferBundles[index]->getBuffer(index2);
  }
  std::shared_ptr<Buffer> getPostProcessingBuffer(int index) {
    return _postProcessingBufferBundle->getBuffer(index);
  }

  std::shared_ptr<Buffer> getTriangleBuffer(int index) {
    return _triangleBufferBundle->getBuffer(index);
  }
  std::shared_ptr<Buffer> getMaterialBuffer(int index) {
    return _materialBufferBundle->getBuffer(index);
  }
  std::shared_ptr<Buffer> getBvhBuffer(int index) { return _bvhBufferBundle->getBuffer(index); }
  std::shared_ptr<Buffer> getLightsBuffer(int index) {
    return _lightsBufferBundle->getBuffer(index);
  }

private:
  VulkanApplicationContext *_appContext;

  // buffer bundles for ray tracing, temporal filtering, and blur filtering
  std::unique_ptr<BufferBundle> _globalBufferBundle;
  std::unique_ptr<BufferBundle> _gradientProjectionBufferBundle;
  std::unique_ptr<BufferBundle> _rtxBufferBundle;
  std::unique_ptr<BufferBundle> _temperalFilterBufferBundle;
  std::vector<std::unique_ptr<BufferBundle>> _stratumFilterBufferBundle;
  std::unique_ptr<BufferBundle> _varianceBufferBundle;
  std::vector<std::unique_ptr<BufferBundle>> _blurFilterBufferBundles;
  std::unique_ptr<BufferBundle> _postProcessingBufferBundle;

  std::unique_ptr<BufferBundle> _triangleBufferBundle;
  std::unique_ptr<BufferBundle> _materialBufferBundle;
  std::unique_ptr<BufferBundle> _bvhBufferBundle;
  std::unique_ptr<BufferBundle> _lightsBufferBundle;
};