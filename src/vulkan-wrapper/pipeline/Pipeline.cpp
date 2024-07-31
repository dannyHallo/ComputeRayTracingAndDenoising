#include "Pipeline.hpp"
#include "../descriptor-set/DescriptorSetBundle.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "file-watcher/ShaderChangeListener.hpp"
#include "scheduler/Scheduler.hpp"
#include "utils/logger/Logger.hpp"

#include <map>
#include <vector>

static const std::map<VkShaderStageFlags, VkPipelineBindPoint> kShaderStageFlagsToBindPoint{
    {VK_SHADER_STAGE_VERTEX_BIT, VK_PIPELINE_BIND_POINT_GRAPHICS},
    {VK_SHADER_STAGE_FRAGMENT_BIT, VK_PIPELINE_BIND_POINT_GRAPHICS},
    {VK_SHADER_STAGE_COMPUTE_BIT, VK_PIPELINE_BIND_POINT_COMPUTE}};

Pipeline::Pipeline(VulkanApplicationContext *appContext, Logger *logger, Scheduler *scheduler,
                   std::string fullPathToShaderSourceCode, DescriptorSetBundle *descriptorSetBundle,
                   VkShaderStageFlags shaderStageFlags, ShaderChangeListener *shaderChangeListener)
    : _appContext(appContext), _logger(logger), _scheduler(scheduler),
      _shaderChangeListener(shaderChangeListener), _descriptorSetBundle(descriptorSetBundle),
      _fullPathToShaderSourceCode(std::move(fullPathToShaderSourceCode)),
      _shaderStageFlags(shaderStageFlags) {

  if (_shaderChangeListener != nullptr) {
    _shaderChangeListener->addWatchingItem(this);
  }
}

Pipeline::~Pipeline() {
  vkDestroyShaderModule(_appContext->getDevice(), _cachedShaderModule, nullptr);
  _cleanupPipelineAndLayout();

  if (_shaderChangeListener != nullptr) {
    _shaderChangeListener->removeWatchingItem(this);
  }
}

void Pipeline::_cleanupPipelineAndLayout() {
  if (_pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(_appContext->getDevice(), _pipelineLayout, nullptr);
    _pipelineLayout = VK_NULL_HANDLE;
  }
  if (_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(_appContext->getDevice(), _pipeline, nullptr);
    _pipeline = VK_NULL_HANDLE;
  }
}

void Pipeline::_cleanupShaderModule() {
  if (_cachedShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(_appContext->getDevice(), _cachedShaderModule, nullptr);
    _cachedShaderModule = VK_NULL_HANDLE;
  }
}

void Pipeline::updateDescriptorSetBundle(DescriptorSetBundle *descriptorSetBundle) {
  _descriptorSetBundle = descriptorSetBundle;
  build();
}

VkShaderModule Pipeline::_createShaderModule(const std::vector<uint32_t> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pCode    = code.data();
  createInfo.codeSize = sizeof(uint32_t) * code.size();

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  vkCreateShaderModule(_appContext->getDevice(), &createInfo, nullptr, &shaderModule);
  return shaderModule;
}

void Pipeline::_bind(VkCommandBuffer commandBuffer, size_t currentFrame) {
  vkCmdBindDescriptorSets(commandBuffer, kShaderStageFlagsToBindPoint.at(_shaderStageFlags),
                          _pipelineLayout, 0, 1,
                          &_descriptorSetBundle->getDescriptorSet(currentFrame), 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
}
