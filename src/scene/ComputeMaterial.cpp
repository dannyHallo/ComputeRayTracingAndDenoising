#include "ComputeMaterial.h"
#include "utils/Logger.h"
#include "utils/Readfile.h"

#include <memory>
#include <vector>

// loads the compute shader, creates descriptor set
void ComputeMaterial::init() {
  Logger::print("Initing compute material {}", mComputeShaderPath.c_str());
  initDescriptorSetLayout();
  initComputePipeline(mComputeShaderPath);
  initDescriptorPool();
  initDescriptorSets();
}

void ComputeMaterial::initComputePipeline(
    const std::string &computeShaderPath) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts    = &mDescriptorSetLayout;

  VkResult result = vkCreatePipelineLayout(
      VulkanApplicationContext::getInstance()->getDevice(), &pipelineLayoutInfo,
      nullptr, &mPipelineLayout);
  Logger::checkStep("vkCreatePipelineLayout", result);

  VkShaderModule shaderModule = createShaderModule(readFile(computeShaderPath));

  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = shaderModule;
  shaderStageInfo.pName =
      "main"; // name of the entry function of current shader

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = mPipelineLayout;
  computePipelineCreateInfo.flags  = 0;
  computePipelineCreateInfo.stage  = shaderStageInfo;

  result = vkCreateComputePipelines(
      VulkanApplicationContext::getInstance()->getDevice(), VK_NULL_HANDLE, 1,
      &computePipelineCreateInfo, nullptr, &mPipeline);
  Logger::checkStep("vkCreateComputePipelines", result);

  // since we have created the pipeline, we can destroy the shader module
  vkDestroyShaderModule(VulkanApplicationContext::getInstance()->getDevice(),
                        shaderModule, nullptr);
}

void ComputeMaterial::bind(VkCommandBuffer commandBuffer, size_t currentFrame) {
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          mPipelineLayout, 0, 1, &mDescriptorSets[currentFrame],
                          0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
}