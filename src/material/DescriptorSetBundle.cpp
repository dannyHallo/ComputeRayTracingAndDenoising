#include "DescriptorSetBundle.hpp"

#include <cassert>

DescriptorSetBundle::~DescriptorSetBundle() {
  vkDestroyDescriptorSetLayout(_appContext->getDevice(), _descriptorSetLayout, nullptr);
  vkDestroyDescriptorPool(_appContext->getDevice(), _descriptorPool, nullptr);
}

void DescriptorSetBundle::bindUniformBufferBundle(uint32_t bindingNo, BufferBundle *bufferBundle) {
  assert(_boundedSlots.find(bindingNo) == _boundedSlots.end() && "binding socket duplicated");
  assert(bufferBundle->getBundleSize() == _bundleSize &&
         "the size of the uniform buffer bundle must be the same as the descriptor set bundle");

  _boundedSlots.insert(bindingNo);
  _uniformBufferBundles.emplace_back(bindingNo, bufferBundle);
}

void DescriptorSetBundle::bindStorageImage(uint32_t bindingNo, Image *storageImage) {
  // just like the func above
  assert(_boundedSlots.find(bindingNo) == _boundedSlots.end() && "binding socket duplicated");

  _boundedSlots.insert(bindingNo);
  _storageImages.emplace_back(bindingNo, storageImage);
}

// storage buffers are only changed by GPU rather than CPU, and their size is big, they cannot be
// bundled
void DescriptorSetBundle::bindStorageBuffer(uint32_t bindingNo, Buffer *buffer) {
  assert(_boundedSlots.find(bindingNo) == _boundedSlots.end() && "binding socket duplicated");

  _boundedSlots.insert(bindingNo);
  _storageBuffers.emplace_back(bindingNo, buffer);
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

  uint32_t uniformBufferSize = _uniformBufferBundles.size() * _bundleSize;
  if (uniformBufferSize > 0) {
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferSize});
  }

  uint32_t storageImageSize = _storageImages.size();
  if (storageImageSize > 0) {
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storageImageSize});
  }

  uint32_t storageBufferSize = _storageBuffers.size();
  if (storageBufferSize > 0) {
    poolSizes.emplace_back(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferSize});
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  // the max number of descriptor sets that can be allocated from this pool
  poolInfo.maxSets       = _bundleSize;
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.poolSizeCount = poolSizes.size();

  VkResult result =
      vkCreateDescriptorPool(_appContext->getDevice(), &poolInfo, nullptr, &_descriptorPool);
  assert(result == VK_SUCCESS && "Material::initDescriptorPool: failed to create descriptor pool");
}

void DescriptorSetBundle::_createDescriptorSetLayout() {
  // creates descriptor set layout that will be used to create every descriptor
  // set features are extracted from buffer bundles, not buffers, thus to be
  // reused in descriptor set creation
  std::vector<VkDescriptorSetLayoutBinding> bindings{};

  for (auto const &[bindingNo, bufferBundle] : _uniformBufferBundles) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = bindingNo;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags      = _shaderStageFlags;
    bindings.push_back(uboLayoutBinding);
  }

  for (auto const &[bindingNo, storageImage] : _storageImages) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding         = bindingNo;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    samplerLayoutBinding.stageFlags      = _shaderStageFlags;
    bindings.push_back(samplerLayoutBinding);
  }

  for (auto const &[bindingNo, storageBuffer] : _storageBuffers) {
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

  VkResult result = vkCreateDescriptorSetLayout(_appContext->getDevice(), &layoutInfo, nullptr,
                                                &_descriptorSetLayout);
  assert(result == VK_SUCCESS &&
         "Material::initDescriptorSetLayout: failed to create descriptor set layout");
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
    auto const &[bindingNo, bufferBundle] = _uniformBufferBundles[i];
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
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
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = dstSet;
    descriptorWrite.dstBinding      = bindingNo;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo      = &storageImageInfos[i];
    descriptorWrites.push_back(descriptorWrite);
  }

  std::vector<VkDescriptorBufferInfo> storageBufferInfos{};
  storageBufferInfos.reserve(_storageBuffers.size());
  for (auto const &[_, buffer] : _storageBuffers) {
    storageBufferInfos.push_back(buffer->getDescriptorInfo());
  }
  for (uint32_t i = 0; i < _storageBuffers.size(); i++) {
    auto const &[bindingNo, buffer] = _storageBuffers[i];
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
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
  allocInfo.descriptorSetCount = _bundleSize;
  allocInfo.pSetLayouts        = layouts.data();

  _descriptorSets.resize(_bundleSize);
  VkResult res =
      vkAllocateDescriptorSets(_appContext->getDevice(), &allocInfo, _descriptorSets.data());
  assert(res == VK_SUCCESS && "Material::initDescriptorSets: failed to allocate descriptor sets");

  for (uint32_t j = 0; j < _bundleSize; j++) {
    _createDescriptorSet(j);
  }
}
