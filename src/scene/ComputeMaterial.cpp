#include "ComputeMaterial.h"
#include "utils/readfile.h"

#include <memory>
#include <vector>

ComputeMaterial::ComputeMaterial(const std::string &computeShaderPath) : Material(), m_computeShaderPath(computeShaderPath) {
  m_descriptorSetsSize = static_cast<uint32_t>(VulkanGlobal::context.getSwapchainImages().size());
  m_initialized        = false;
}

// loads the compute shader, creates descriptor set
void ComputeMaterial::init() {
  if (m_initialized) {
    return;
  }
  __initDescriptorSetLayout();
  __initComputePipeline(m_computeShaderPath);
  __initDescriptorPool();
  __initDescriptorSets();
  m_initialized = true;
}

void ComputeMaterial::__initComputePipeline(const std::string &computeShaderPath) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts    = &m_descriptorSetLayout;

  if (vkCreatePipelineLayout(VulkanGlobal::context.getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkShaderModule shaderModule = __createShaderModule(readFile(computeShaderPath));

  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = shaderModule;
  shaderStageInfo.pName  = "main"; // the entry function of the shader code

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = m_pipelineLayout;
  computePipelineCreateInfo.flags  = 0;
  computePipelineCreateInfo.stage  = shaderStageInfo;

  if (vkCreateComputePipelines(VulkanGlobal::context.getDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr,
                               &m_pipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(VulkanGlobal::context.getDevice(), shaderModule, nullptr);
}

void ComputeMaterial::bind(VkCommandBuffer &commandBuffer, size_t currentFrame) {
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSets[currentFrame],
                          0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
}