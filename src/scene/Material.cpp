#include "Material.h"
#include "utils/logger.h"
#include "utils/readfile.h"

#include <memory>
#include <vector>

VkShaderModule Material::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  VkResult result =
      vkCreateShaderModule(VulkanApplicationContext::getInstance()->getDevice(), &createInfo, nullptr, &shaderModule);
  logger::checkStep("vkCreateShaderModule", result);
  return shaderModule;
}

Material::~Material() {
  vkDestroyDescriptorSetLayout(VulkanApplicationContext::getInstance()->getDevice(), mDescriptorSetLayout, nullptr);
  vkDestroyPipeline(VulkanApplicationContext::getInstance()->getDevice(), mPipeline, nullptr);
  vkDestroyPipelineLayout(VulkanApplicationContext::getInstance()->getDevice(), mPipelineLayout, nullptr);

  // mDescriptorSets be automatically freed when the descriptor pool is destroyed
  vkDestroyDescriptorPool(VulkanApplicationContext::getInstance()->getDevice(), mDescriptorPool, nullptr);
}

void Material::addStorageImage(Image *storageImage) { mStorageImages.emplace_back(storageImage); }

void Material::addUniformBufferBundle(BufferBundle *uniformBufferBundle) {
  mUniformBufferBundles.emplace_back(uniformBufferBundle);
}

void Material::addStorageBufferBundle(BufferBundle *storageBufferBundle) {
  mStorageBufferBundles.emplace_back(storageBufferBundle);
}

void Material::initDescriptorSetLayout() {
  // creates descriptor set layout that will be used to create every descriptor set
  // features are extracted from buffer bundles, not buffers, thus to be reused in descriptor set creation
  std::vector<VkDescriptorSetLayoutBinding> bindings{};
  uint32_t bindingNo = 0;

  // each binding is for each buffer BUNDLE
  for (int i = 0; i < mUniformBufferBundles.size(); i++) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = bindingNo++;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags      = mShaderStageFlags;
    bindings.push_back(uboLayoutBinding);
  }

  for (int i = 0; i < mStorageImages.size(); i++) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding         = bindingNo++;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    samplerLayoutBinding.stageFlags      = mShaderStageFlags;
    bindings.push_back(samplerLayoutBinding);
  }

  for (int i = 0; i < mStorageBufferBundles.size(); i++) {
    VkDescriptorSetLayoutBinding storageBufferBinding{};
    storageBufferBinding.binding         = bindingNo++;
    storageBufferBinding.descriptorCount = 1;
    storageBufferBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageBufferBinding.stageFlags      = mShaderStageFlags;
    bindings.push_back(storageBufferBinding);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  VkResult result = vkCreateDescriptorSetLayout(VulkanApplicationContext::getInstance()->getDevice(), &layoutInfo,
                                                nullptr, &mDescriptorSetLayout);
  logger::checkStep("vkCreateDescriptorSetLayout", result);
}

void Material::initDescriptorPool() {
  std::vector<VkDescriptorPoolSize> poolSizes{};
  poolSizes.reserve(mUniformBufferBundles.size() + mStorageImages.size() + mStorageBufferBundles.size());

  // https://www.reddit.com/r/vulkan/comments/8u9zqr/having_trouble_understanding_descriptor_pool/
  // pool sizes info indicates how many descriptors of a certain type can be allocated from the pool - NOT THE SET!
  for (size_t i = 0; i < mUniformBufferBundles.size(); i++) {
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mSwapchainSize});
  }

  for (size_t i = 0; i < mStorageImages.size(); i++) {
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, mSwapchainSize});
  }

  for (size_t i = 0; i < mStorageBufferBundles.size(); i++) {
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mSwapchainSize});
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  // the max number of descriptor sets that can be allocated from this pool
  poolInfo.maxSets = mSwapchainSize;

  VkResult result = vkCreateDescriptorPool(VulkanApplicationContext::getInstance()->getDevice(), &poolInfo, nullptr,
                                           &mDescriptorPool);
  logger::checkStep("vkCreateDescriptorPool", result);
}

// chop the buffer bundles here, and create the descriptor sets for each of the swapchains buffers are loaded to
// descriptor sets
void Material::initDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(mSwapchainSize, mDescriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = mDescriptorPool;
  allocInfo.descriptorSetCount = mSwapchainSize;
  allocInfo.pSetLayouts        = layouts.data();

  mDescriptorSets.resize(mSwapchainSize);
  if (vkAllocateDescriptorSets(VulkanApplicationContext::getInstance()->getDevice(), &allocInfo,
                               mDescriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t j = 0; j < mSwapchainSize; j++) {
    VkDescriptorSet &dstSet = mDescriptorSets[j];

    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.reserve(mUniformBufferBundles.size() + mStorageImages.size() + mStorageBufferBundles.size());

    uint32_t bindingNo = 0;

    std::vector<VkDescriptorBufferInfo> uniformBufferInfos{};
    uniformBufferInfos.reserve(mUniformBufferBundles.size());
    for (auto &mUniformBufferBundleDescriptor : mUniformBufferBundles) {
      uniformBufferInfos.emplace_back(mUniformBufferBundleDescriptor->getBuffer(j)->getDescriptorInfo());
    }

    for (size_t i = 0; i < mUniformBufferBundles.size(); i++) {
      VkWriteDescriptorSet descriptorSet{};
      descriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorSet.dstSet          = dstSet;
      descriptorSet.dstBinding      = bindingNo++;
      descriptorSet.dstArrayElement = 0;
      descriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorSet.descriptorCount = 1;
      descriptorSet.pBufferInfo     = &uniformBufferInfos[i];
      descriptorWrites.push_back(descriptorSet);
    }

    std::vector<VkDescriptorImageInfo> storageImageInfos{};
    storageImageInfos.reserve(mStorageImages.size());
    for (auto &mStorageImageDescriptor : mStorageImages) {
      storageImageInfos.push_back(mStorageImageDescriptor->getDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL));
    }

    for (size_t i = 0; i < mStorageImages.size(); i++) {
      VkWriteDescriptorSet descriptorSet{};
      descriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorSet.dstSet          = dstSet;
      descriptorSet.dstBinding      = bindingNo++;
      descriptorSet.dstArrayElement = 0;
      descriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorSet.descriptorCount = 1;
      descriptorSet.pImageInfo      = &storageImageInfos[i];
      descriptorWrites.push_back(descriptorSet);
    }

    std::vector<VkDescriptorBufferInfo> storageBufferInfos{};
    storageBufferInfos.reserve(mStorageBufferBundles.size());
    for (auto &mStorageBufferBundleDescriptor : mStorageBufferBundles) {
      storageBufferInfos.push_back(mStorageBufferBundleDescriptor->getBuffer(j)->getDescriptorInfo());
    }

    for (size_t i = 0; i < mStorageBufferBundles.size(); i++) {
      VkWriteDescriptorSet descriptorSet{};
      descriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorSet.dstSet          = dstSet;
      descriptorSet.dstBinding      = bindingNo++;
      descriptorSet.dstArrayElement = 0;
      descriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorSet.descriptorCount = 1;
      descriptorSet.pBufferInfo     = &storageBufferInfos[i];

      descriptorWrites.push_back(descriptorSet);
    }

    vkUpdateDescriptorSets(VulkanApplicationContext::getInstance()->getDevice(),
                           static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }
}