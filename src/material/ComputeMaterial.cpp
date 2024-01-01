#include "ComputeMaterial.hpp"
#include "utils/io/Readfile.hpp"
#include "utils/logger/Logger.hpp"

#include <cassert>
#include <memory>
#include <vector>

// loads the compute shader, creates descriptor set
void ComputeMaterial::init(Logger *logger) {
  logger->print("Initing compute material {}", _computeShaderPath.c_str());
  _createDescriptorSetLayout();
  // must be done after _createDescriptorSetLayout(), we need that layout
  _createComputePipeline();
  _createDescriptorPool();
  _createDescriptorSets();
}

void ComputeMaterial::_createComputePipeline() {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts    = &_descriptorSetLayout;

  VkResult result = vkCreatePipelineLayout(_appContext->getDevice(), &pipelineLayoutInfo, nullptr,
                                           &_pipelineLayout);
  assert(result == VK_SUCCESS && "failed to create pipeline layout");

  VkShaderModule shaderModule = _createShaderModule(readFile(_computeShaderPath));

  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = shaderModule;
  shaderStageInfo.pName  = "main"; // name of the entry function of current shader

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = _pipelineLayout;
  computePipelineCreateInfo.flags  = 0;
  computePipelineCreateInfo.stage  = shaderStageInfo;

  result = vkCreateComputePipelines(_appContext->getDevice(), VK_NULL_HANDLE, 1,
                                    &computePipelineCreateInfo, nullptr, &_pipeline);
  assert(result == VK_SUCCESS && "failed to create compute pipeline");

  // since we have created the pipeline, we can destroy the shader module
  vkDestroyShaderModule(_appContext->getDevice(), shaderModule, nullptr);
}

void ComputeMaterial::bind(VkCommandBuffer commandBuffer, size_t currentFrame) {
  _bind(VK_PIPELINE_BIND_POINT_COMPUTE, commandBuffer, currentFrame);
}