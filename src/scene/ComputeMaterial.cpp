#include "ComputeMaterial.h"
#include "utils/readfile.h"

#include <memory>
#include <vector>

// loads the compute shader, creates descriptor set
void ComputeMaterial::init() {
  if (mInitialized) return;

  initDescriptorSetLayout();
  initComputePipeline(mComputeShaderPath);
  initDescriptorPool();
  initDescriptorSets();
  
  mInitialized = true;
}

void ComputeMaterial::initComputePipeline(const std::string &computeShaderPath) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts    = &mDescriptorSetLayout;

  if (vkCreatePipelineLayout(vulkanApplicationContext.getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkShaderModule shaderModule = createShaderModule(readFile(computeShaderPath));

  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = shaderModule;
  shaderStageInfo.pName  = "main"; // the entry function of the shader code

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = mPipelineLayout;
  computePipelineCreateInfo.flags  = 0;
  computePipelineCreateInfo.stage  = shaderStageInfo;

  if (vkCreateComputePipelines(vulkanApplicationContext.getDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo,
                               nullptr, &mPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(vulkanApplicationContext.getDevice(), shaderModule, nullptr);
}

void ComputeMaterial::bind(VkCommandBuffer &commandBuffer, size_t currentFrame) {
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1,
                          &mDescriptorSets[currentFrame], 0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
}