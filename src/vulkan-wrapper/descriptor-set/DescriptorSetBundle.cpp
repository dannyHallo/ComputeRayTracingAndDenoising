#include "DescriptorSetBundle.hpp"

#include "app-context/VulkanApplicationContext.hpp"

#include "../memory/Buffer.hpp"
#include "../memory/BufferBundle.hpp"

#include <cassert>

DescriptorSetBundle::~DescriptorSetBundle() {
  vkDestroyDescriptorSetLayout(_appContext->getDevice(), _descriptorSetLayout, nullptr);
  vkDestroyDescriptorPool(_appContext->getDevice(), _descriptorPool, nullptr);
}

void DescriptorSetBundle::bindUniformBufferBundle(uint32_t bindingSlot,
                                                  BufferBundle *bufferBundle) {
  assert(_boundedSlots.find(bindingSlot) == _boundedSlots.end() && "binding socket duplicated");
  assert(bufferBundle->getBundleSize() == _bundleSize &&
         "the size of the uniform buffer bundle must be the same as the descriptor set bundle");

  _boundedSlots.insert(bindingSlot);
  _uniformBufferBundles.emplace_back(bindingSlot, bufferBundle);
}

void DescriptorSetBundle::bindStorageImage(uint32_t bindingSlot, Image *storageImage) {
  // just like the func above
  assert(_boundedSlots.find(bindingSlot) == _boundedSlots.end() && "binding socket duplicated");

  _boundedSlots.insert(bindingSlot);
  _storageImages.emplace_back(bindingSlot, storageImage);
}

void DescriptorSetBundle::bindImageSampler(uint32_t bindingSlot, Image *storageImage) {
  // just like the func above
  assert(_boundedSlots.find(bindingSlot) == _boundedSlots.end() && "binding socket duplicated");

  _boundedSlots.insert(bindingSlot);
  _imageSamplers.emplace_back(bindingSlot, storageImage);
}

// storage buffers are only changed by GPU rather than CPU, and their size is big, they cannot be
// bundled
void DescriptorSetBundle::bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) {
  assert(_boundedSlots.find(bindingSlot) == _boundedSlots.end() && "binding socket duplicated");

  _boundedSlots.insert(bindingSlot);
  _storageBuffers.emplace_back(bindingSlot, buffer);
}

void DescriptorSetBundle::create() {
  _createDescriptorPool();
  _createDescriptorSetLayout();
  _createDescriptorSets();
}

void DescriptorSetBundle::_createDescriptorPool() {
  std::vector<VkDescriptorPoolSize> poolSizes{};

  // https://www.reddit.com/r/vulkan/comments/8u9zqr/having_trouble_understanding_descriptor_pool/
  // pool sizes info indicates how many descriptors of a certain type can be
  // allocated from the pool - NOT THE SET!

  auto uniformBufferSize = static_cast<uint32_t>(_uniformBufferBundles.size() * _bundleSize);
  if (uniformBufferSize > 0) {
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferSize});
  }

  auto storageImageSize = static_cast<uint32_t>(_storageImages.size());
  if (storageImageSize > 0) {
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storageImageSize});
  }

  auto imageSamplerSize = static_cast<uint32_t>(_imageSamplers.size());
  if (imageSamplerSize > 0) {
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageSamplerSize});
  }

  auto storageBufferSize = static_cast<uint32_t>(_storageBuffers.size());
  if (storageBufferSize > 0) {
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferSize});
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  // the max number of descriptor sets that can be allocated from this pool
  poolInfo.maxSets       = static_cast<uint32_t>(_bundleSize);
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());

  vkCreateDescriptorPool(_appContext->getDevice(), &poolInfo, nullptr, &_descriptorPool);
}

void DescriptorSetBundle::_createDescriptorSetLayout() {
  // creates descriptor set layout that will be used to create every descriptor
  // set features are extracted from buffer bundles, not buffers, thus to be
  // reused in descriptor set creation
  std::vector<VkDescriptorSetLayoutBinding> bindings{};

  for (auto const &[bindingNo, _] : _uniformBufferBundles) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = bindingNo;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags      = _shaderStageFlags;
    bindings.push_back(uboLayoutBinding);
  }

  for (auto const &[bindingNo, _] : _storageImages) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding         = bindingNo;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    samplerLayoutBinding.stageFlags      = _shaderStageFlags;
    bindings.push_back(samplerLayoutBinding);
  }

  for (auto const &[bindingNo, _] : _imageSamplers) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding         = bindingNo;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags      = _shaderStageFlags;
    bindings.push_back(samplerLayoutBinding);
  }

  for (auto const &[bindingNo, _] : _storageBuffers) {
    VkDescriptorSetLayoutBinding storageBufferBinding{};
    storageBufferBinding.binding         = bindingNo;
    storageBufferBinding.descriptorCount = 1;
    storageBufferBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageBufferBinding.stageFlags      = _shaderStageFlags;
    bindings.push_back(storageBufferBinding);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  vkCreateDescriptorSetLayout(_appContext->getDevice(), &layoutInfo, nullptr,
                              &_descriptorSetLayout);
}

void DescriptorSetBundle::_createDescriptorSet(uint32_t descriptorSetIndex) {
  VkDescriptorSet &dstSet = _descriptorSets[descriptorSetIndex];

  std::vector<VkWriteDescriptorSet> descriptorWrites{};
  descriptorWrites.reserve(_boundedSlots.size());

  std::vector<VkDescriptorBufferInfo> uniformBufferInfos{};
  uniformBufferInfos.reserve(_uniformBufferBundles.size());
  for (auto const &[_, bufferBundle] : _uniformBufferBundles) {
    uniformBufferInfos.push_back(bufferBundle->getBuffer(descriptorSetIndex)->getDescriptorInfo());
  }
  for (uint32_t i = 0; i < _uniformBufferBundles.size(); i++) {
    auto const &[bindingNo, _] = _uniformBufferBundles[i];
    VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet          = dstSet;
    descriptorWrite.dstBinding      = bindingNo;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo     = &uniformBufferInfos[i];
    descriptorWrites.push_back(descriptorWrite);
  }

  std::vector<VkDescriptorImageInfo> storageImageInfos{};
  storageImageInfos.reserve(_storageImages.size());
  for (auto const &[_, storageImage] : _storageImages) {
    storageImageInfos.push_back(storageImage->getDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL));
  }
  for (uint32_t i = 0; i < _storageImages.size(); i++) {
    auto const &[bindingNo, storageImage] = _storageImages[i];
    VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet          = dstSet;
    descriptorWrite.dstBinding      = bindingNo;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo      = &storageImageInfos[i];
    descriptorWrites.push_back(descriptorWrite);
  }

  std::vector<VkDescriptorImageInfo> imageSamplerInfos{};
  imageSamplerInfos.reserve(_imageSamplers.size());
  for (auto const &[_, storageImage] : _imageSamplers) {
    // or VK_IMAGE_LAYOUT_GENERAL TODO: check
    imageSamplerInfos.push_back(storageImage->getDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL));
  }
  for (uint32_t i = 0; i < _imageSamplers.size(); i++) {
    auto const &[bindingNo, storageImage] = _imageSamplers[i];
    VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet          = dstSet;
    descriptorWrite.dstBinding      = bindingNo;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo      = &imageSamplerInfos[i];
    descriptorWrites.push_back(descriptorWrite);
  }

  std::vector<VkDescriptorBufferInfo> storageBufferInfos{};
  storageBufferInfos.reserve(_storageBuffers.size());
  for (auto const &[_, buffer] : _storageBuffers) {
    storageBufferInfos.push_back(buffer->getDescriptorInfo());
  }
  for (uint32_t i = 0; i < _storageBuffers.size(); i++) {
    auto const &[bindingNo, buffer] = _storageBuffers[i];
    VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet          = dstSet;
    descriptorWrite.dstBinding      = bindingNo;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo     = &storageBufferInfos[i];
    descriptorWrites.push_back(descriptorWrite);
  }

  vkUpdateDescriptorSets(_appContext->getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

void DescriptorSetBundle::_createDescriptorSets() {
  // set bundle uses identical layout, but with different data
  std::vector<VkDescriptorSetLayout> layouts(_bundleSize, _descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = _descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(_bundleSize);
  allocInfo.pSetLayouts        = layouts.data();

  _descriptorSets.resize(_bundleSize);
  vkAllocateDescriptorSets(_appContext->getDevice(), &allocInfo, _descriptorSets.data());

  for (uint32_t j = 0; j < _bundleSize; j++) {
    _createDescriptorSet(j);
  }
}
