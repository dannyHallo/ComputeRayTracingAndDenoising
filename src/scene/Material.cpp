#include "Material.h"
#include "utils/logger.h"
#include "utils/readfile.h"

#include <memory>
#include <vector>

Material::~Material() {
  vkDestroyDescriptorSetLayout(VulkanApplicationContext::getInstance()->getDevice(), mDescriptorSetLayout, nullptr);
  vkDestroyPipeline(VulkanApplicationContext::getInstance()->getDevice(), mPipeline, nullptr);
  vkDestroyPipelineLayout(VulkanApplicationContext::getInstance()->getDevice(), mPipelineLayout, nullptr);
  vkDestroyDescriptorPool(VulkanApplicationContext::getInstance()->getDevice(), mDescriptorPool, nullptr);
}

VkShaderModule Material::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  VkResult result =
      vkCreateShaderModule(VulkanApplicationContext::getInstance()->getDevice(), &createInfo, nullptr, &shaderModule);
  logger::checkStep("vkCreateShaderModule", result);
  return shaderModule;
}

void Material::addUniformBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle,
                                      VkShaderStageFlags shaderStageFlags) {
  mUniformBufferBundleDescriptors.emplace_back(Descriptor<BufferBundle>{bufferBundle, shaderStageFlags});
}

void Material::addStorageBufferBundle(const std::shared_ptr<BufferBundle> &bufferBundle,
                                      VkShaderStageFlags shaderStageFlags) {
  mStorageBufferBundleDescriptors.emplace_back(Descriptor<BufferBundle>{bufferBundle, shaderStageFlags});
}

void Material::addStorageImage(const std::shared_ptr<Image> &image, VkShaderStageFlags shaderStageFlags) {
  mStorageImageDescriptors.emplace_back(Descriptor<Image>{image, shaderStageFlags});
}

void Material::initDescriptorSetLayout() {
  // creates descriptor set layout that will be used to create every descriptor set
  // features are extracted from buffer bundles, not buffers, thus to be reused in descriptor set creation
  std::vector<VkDescriptorSetLayoutBinding> bindings{};
  uint32_t bindingNo = 0;

  // each binding is for each buffer BUNDLE
  for (const auto &d : mUniformBufferBundleDescriptors) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = bindingNo++;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags      = d.shaderStageFlags;
    bindings.push_back(uboLayoutBinding);
  }

  for (const auto &s : mStorageImageDescriptors) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding         = bindingNo++;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    samplerLayoutBinding.stageFlags      = s.shaderStageFlags;
    bindings.push_back(samplerLayoutBinding);
  }

  for (const auto &s : mStorageBufferBundleDescriptors) {
    VkDescriptorSetLayoutBinding storageBufferBinding{};
    storageBufferBinding.binding         = bindingNo++;
    storageBufferBinding.descriptorCount = 1;
    storageBufferBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageBufferBinding.stageFlags      = s.shaderStageFlags;
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
  poolSizes.reserve(mUniformBufferBundleDescriptors.size() + mStorageImageDescriptors.size() +
                    mStorageBufferBundleDescriptors.size());

  // https://www.reddit.com/r/vulkan/comments/8u9zqr/having_trouble_understanding_descriptor_pool/
  // pool sizes info indicates how many descriptors of a certain type can be allocated from the pool - NOT THE SET!
  for (size_t i = 0; i < mUniformBufferBundleDescriptors.size(); i++) {
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mSwapchainSize});
  }

  for (size_t i = 0; i < mStorageImageDescriptors.size(); i++) {
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, mSwapchainSize});
  }

  for (size_t i = 0; i < mStorageBufferBundleDescriptors.size(); i++) {
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
    descriptorWrites.reserve(mUniformBufferBundleDescriptors.size() + mStorageImageDescriptors.size() +
                             mStorageBufferBundleDescriptors.size());

    uint32_t bindingNo = 0;

    std::vector<VkDescriptorBufferInfo> uniformBufferInfos{};
    for (size_t i = 0; i < mUniformBufferBundleDescriptors.size(); i++) {
      uniformBufferInfos.emplace_back(mUniformBufferBundleDescriptors[i].data->getBuffer(j)->getDescriptorInfo());
    }
    for (size_t i = 0; i < mUniformBufferBundleDescriptors.size(); i++) {
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
    for (size_t i = 0; i < mStorageImageDescriptors.size(); i++) {
      storageImageInfos.push_back(mStorageImageDescriptors[i].data->getDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL));
    }
    for (size_t i = 0; i < mStorageImageDescriptors.size(); i++) {
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
    for (size_t i = 0; i < mStorageBufferBundleDescriptors.size(); i++) {
      storageBufferInfos.push_back(mStorageBufferBundleDescriptors[i].data->getBuffer(j)->getDescriptorInfo());
    }
    for (size_t i = 0; i < mStorageBufferBundleDescriptors.size(); i++) {
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