#include "Material.h"
#include "utils/logger.h"
#include "utils/readfile.h"

#include <memory>
#include <vector>

// Material::Material(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
//     : mFragmentShaderPath(fragmentShaderPath), mVertexShaderPath(vertexShaderPath), mInitialized(false),
//       mDescriptorSetsSize(static_cast<uint32_t>(vulkanApplicationContext.getSwapchainSize())) {}

Material::~Material() {
  vkDestroyDescriptorSetLayout(vulkanApplicationContext.getDevice(), mDescriptorSetLayout, nullptr);
  vkDestroyPipeline(vulkanApplicationContext.getDevice(), mPipeline, nullptr);
  vkDestroyPipelineLayout(vulkanApplicationContext.getDevice(), mPipelineLayout, nullptr);
  vkDestroyDescriptorPool(vulkanApplicationContext.getDevice(), mDescriptorPool, nullptr);
}

VkShaderModule Material::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(vulkanApplicationContext.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
  return shaderModule;
}

// void Material::addTexture(const std::shared_ptr<Texture> &texture, VkShaderStageFlags shaderStageFlags) {
//   mTextureDescriptors.emplace_back(Descriptor<BufferBundle>{texture, shaderStageFlags});
// }

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

// // Initialize material when adding to a scene.
// void Material::init(const VkRenderPass &renderPass) {
//   if (mInitialized) {
//     return;
//   }
//   initDescriptorSetLayout();
//   initPipeline(vulkanApplicationContext.getSwapchainExtent(), renderPass, mVertexShaderPath, mFragmentShaderPath);
//   initDescriptorPool();
//   initDescriptorSets();
//   mInitialized = true;
// }

// void Material::initPipeline(const VkExtent2D &swapChainExtent, const VkRenderPass &renderPass,
//                             std::string vertexShaderPath, std::string fragmentShaderPath) {
//   auto vertShaderCode             = readFile(vertexShaderPath);
//   auto fragShaderCode             = readFile(fragmentShaderPath);
//   VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
//   VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

//   VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
//   vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//   vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
//   vertShaderStageInfo.module = vertShaderModule;
//   vertShaderStageInfo.pName  = "main";

//   VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
//   fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//   fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
//   fragShaderStageInfo.module = fragShaderModule;
//   fragShaderStageInfo.pName  = "main";

//   VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

//   VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//   vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//   vertexInputInfo.vertexBindingDescriptionCount   = 0;
//   vertexInputInfo.pVertexBindingDescriptions      = nullptr; // Optional
//   vertexInputInfo.vertexAttributeDescriptionCount = 0;
//   vertexInputInfo.pVertexAttributeDescriptions    = nullptr; // Optional
//   auto bindingDescription                         = Vertex::getBindingDescription();
//   auto attributeDescriptions                      = Vertex::getAttributeDescriptions();

//   vertexInputInfo.vertexBindingDescriptionCount   = 1;
//   vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
//   vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
//   vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

//   VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//   inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//   inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//   inputAssembly.primitiveRestartEnable = VK_FALSE;

//   VkViewport viewport{};
//   viewport.x        = 0.0f;
//   viewport.y        = 0.0f;
//   viewport.width    = (float)swapChainExtent.width;
//   viewport.height   = (float)swapChainExtent.height;
//   viewport.minDepth = 0.0f;
//   viewport.maxDepth = 1.0f;

//   VkRect2D scissor{};
//   scissor.offset = {0, 0};
//   scissor.extent = swapChainExtent;

//   VkPipelineViewportStateCreateInfo viewportState{};
//   viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//   viewportState.viewportCount = 1;
//   viewportState.pViewports    = &viewport;
//   viewportState.scissorCount  = 1;
//   viewportState.pScissors     = &scissor;

//   VkPipelineRasterizationStateCreateInfo rasterizer{};
//   rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//   rasterizer.depthClampEnable        = VK_FALSE;
//   rasterizer.rasterizerDiscardEnable = VK_FALSE;
//   rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
//   rasterizer.lineWidth               = 1.0f;
//   rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
//   rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//   rasterizer.depthBiasEnable         = VK_FALSE;
//   rasterizer.depthBiasConstantFactor = 0.0f; // Optional
//   rasterizer.depthBiasClamp          = 0.0f; // Optional
//   rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional

//   VkPipelineMultisampleStateCreateInfo multisampling{};
//   multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//   multisampling.sampleShadingEnable   = VK_FALSE;
//   multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
//   multisampling.minSampleShading      = .2f;      // min fraction for sample shading; closer to one is smoother
//   multisampling.pSampleMask           = nullptr;  // Optional
//   multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
//   multisampling.alphaToOneEnable      = VK_FALSE; // Optional

//   VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//   colorBlendAttachment.colorWriteMask =
//       VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//   colorBlendAttachment.blendEnable = VK_FALSE;

//   VkPipelineColorBlendStateCreateInfo colorBlending{};
//   colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//   colorBlending.logicOpEnable     = VK_FALSE;
//   colorBlending.logicOp           = VK_LOGIC_OP_COPY; // Optional
//   colorBlending.attachmentCount   = 1;
//   colorBlending.pAttachments      = &colorBlendAttachment;
//   colorBlending.blendConstants[0] = 0.0f; // Optional
//   colorBlending.blendConstants[1] = 0.0f; // Optional
//   colorBlending.blendConstants[2] = 0.0f; // Optional
//   colorBlending.blendConstants[3] = 0.0f; // Optional

//   VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

//   VkPipelineDynamicStateCreateInfo dynamicState{};
//   dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//   dynamicState.dynamicStateCount = 2;
//   dynamicState.pDynamicStates    = dynamicStates;

//   VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//   pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//   pipelineLayoutInfo.setLayoutCount = 1;
//   pipelineLayoutInfo.pSetLayouts    = &mDescriptorSetLayout;

//   if (vkCreatePipelineLayout(vulkanApplicationContext.getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) !=
//       VK_SUCCESS) {
//     throw std::runtime_error("failed to create pipeline layout!");
//   }

//   VkPipelineDepthStencilStateCreateInfo depthStencil{};
//   depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//   depthStencil.depthTestEnable       = VK_TRUE;
//   depthStencil.depthWriteEnable      = VK_TRUE;
//   depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
//   depthStencil.depthBoundsTestEnable = VK_FALSE;
//   depthStencil.minDepthBounds        = 0.0f; // Optional
//   depthStencil.maxDepthBounds        = 1.0f; // Optional
//   depthStencil.stencilTestEnable     = VK_FALSE;
//   depthStencil.front                 = {}; // Optional
//   depthStencil.back                  = {}; // Optional

//   VkGraphicsPipelineCreateInfo pipelineInfo{};
//   pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//   pipelineInfo.stageCount          = 2;
//   pipelineInfo.pStages             = shaderStages;
//   pipelineInfo.pVertexInputState   = &vertexInputInfo;
//   pipelineInfo.pInputAssemblyState = &inputAssembly;
//   pipelineInfo.pViewportState      = &viewportState;
//   pipelineInfo.pRasterizationState = &rasterizer;
//   pipelineInfo.pMultisampleState   = &multisampling;
//   pipelineInfo.pDepthStencilState  = nullptr; // Optional
//   pipelineInfo.pColorBlendState    = &colorBlending;
//   pipelineInfo.pDynamicState       = nullptr; // Optional
//   pipelineInfo.layout              = mPipelineLayout;
//   pipelineInfo.renderPass          = renderPass;
//   pipelineInfo.subpass             = 0;
//   pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE; // Optional
//   pipelineInfo.basePipelineIndex   = -1;             // Optional
//   pipelineInfo.pDepthStencilState  = &depthStencil;

//   if (vkCreateGraphicsPipelines(vulkanApplicationContext.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
//                                 &mPipeline) != VK_SUCCESS) {
//     throw std::runtime_error("failed to create graphics pipeline!");
//   }

//   vkDestroyShaderModule(vulkanApplicationContext.getDevice(), fragShaderModule, nullptr);
//   vkDestroyShaderModule(vulkanApplicationContext.getDevice(), vertShaderModule, nullptr);
// }

// creates the descriptor pool that the descriptor sets will be allocated into
void Material::initDescriptorPool() {
  std::vector<VkDescriptorPoolSize> poolSizes{};
  // https://www.reddit.com/r/vulkan/comments/8u9zqr/having_trouble_understanding_descriptor_pool/
  // pool sizes info indicates how many descriptors of a certain type can be allocated from the pool - NOT THE SET!
  for (size_t i = 0; i < mUniformBufferBundleDescriptors.size(); i++) {
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mDescriptorSetsSize});
  }

  // for (size_t i = 0; i < mTextureDescriptors.size(); i++) {
  //   poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mDescriptorSetsSize});
  // }

  for (size_t i = 0; i < mStorageImageDescriptors.size(); i++) {
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, mDescriptorSetsSize});
  }

  for (size_t i = 0; i < mStorageBufferBundleDescriptors.size(); i++) {
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mDescriptorSetsSize});
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  // the max number of descriptor sets that can be allocated from this pool
  poolInfo.maxSets = static_cast<uint32_t>(mDescriptorSetsSize);

  VkResult result = vkCreateDescriptorPool(vulkanApplicationContext.getDevice(), &poolInfo, nullptr, &mDescriptorPool);
  logger::checkStep("vkCreateDescriptorPool", result);
}

// creates descriptor set layout that will be used to create every descriptor set
// features are extracted from buffer bundles, not buffers, thus to be reused in descriptor set creation
void Material::initDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  uint32_t bindingNo = 0;

  // each binding is for each buffer BUNDLE
  for (const auto &d : mUniformBufferBundleDescriptors) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding            = bindingNo++;
    uboLayoutBinding.descriptorCount    = 1;
    uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags         = d.shaderStageFlags;
    bindings.push_back(uboLayoutBinding);
  }

  for (const auto &s : mStorageImageDescriptors) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding            = bindingNo++;
    samplerLayoutBinding.descriptorCount    = 1;
    samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    samplerLayoutBinding.stageFlags         = s.shaderStageFlags;
    bindings.push_back(samplerLayoutBinding);
  }

  for (const auto &s : mStorageBufferBundleDescriptors) {
    VkDescriptorSetLayoutBinding storageBufferBinding{};
    storageBufferBinding.binding            = bindingNo++;
    storageBufferBinding.descriptorCount    = 1;
    storageBufferBinding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageBufferBinding.stageFlags         = s.shaderStageFlags;
    bindings.push_back(storageBufferBinding);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  if (vkCreateDescriptorSetLayout(vulkanApplicationContext.getDevice(), &layoutInfo, nullptr, &mDescriptorSetLayout) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

// chop the buffer bundles here, and create the descriptor sets for each of the swapchains
// buffers are loaded to descriptor sets
void Material::initDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(mDescriptorSetsSize, mDescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = mDescriptorPool;
  allocInfo.descriptorSetCount = mDescriptorSetsSize;
  allocInfo.pSetLayouts        = layouts.data();

  mDescriptorSets.resize(mDescriptorSetsSize);
  if (vkAllocateDescriptorSets(vulkanApplicationContext.getDevice(), &allocInfo, mDescriptorSets.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t j = 0; j < mDescriptorSetsSize; j++) {
    VkDescriptorSet &dstSet = mDescriptorSets[j];

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    // descriptorWrites.reserve(mUniformBufferBundleDescriptors.size() + mTextureDescriptors.size() +
    //                          mStorageImageDescriptors.size());

    descriptorWrites.reserve(mUniformBufferBundleDescriptors.size() + mStorageImageDescriptors.size());

    std::vector<VkDescriptorBufferInfo> bufferDescInfos;
    for (size_t i = 0; i < mUniformBufferBundleDescriptors.size(); i++) {
      bufferDescInfos.push_back(mUniformBufferBundleDescriptors[i].data->getBuffer(j)->getDescriptorInfo());
    }

    uint32_t binding = 0;

    for (size_t i = 0; i < mUniformBufferBundleDescriptors.size(); i++) {
      VkWriteDescriptorSet descriptorSet{};
      descriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorSet.dstSet          = dstSet;
      descriptorSet.dstBinding      = binding++;
      descriptorSet.dstArrayElement = 0;
      descriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorSet.descriptorCount = 1;
      descriptorSet.pBufferInfo     = &bufferDescInfos[i];

      descriptorWrites.push_back(descriptorSet);
    }

    // std::vector<VkDescriptorImageInfo> imageInfos;
    // for (size_t i = 0; i < mTextureDescriptors.size(); i++) {
    //   imageInfos.push_back(mTextureDescriptors[i].data->getDescriptorInfo());
    // }

    // for (size_t i = 0; i < mTextureDescriptors.size(); i++) {
    //   VkWriteDescriptorSet descriptorSet{};
    //   descriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //   descriptorSet.dstSet          = dstSet;
    //   descriptorSet.dstBinding      = binding++;
    //   descriptorSet.dstArrayElement = 0;
    //   descriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //   descriptorSet.descriptorCount = 1;
    //   descriptorSet.pImageInfo      = &imageInfos[i];

    //   descriptorWrites.push_back(descriptorSet);
    // }

    std::vector<VkDescriptorImageInfo> storageImageInfos;
    for (size_t i = 0; i < mStorageImageDescriptors.size(); i++) {
      storageImageInfos.push_back(mStorageImageDescriptors[i].data->getDescriptorInfo(VK_IMAGE_LAYOUT_GENERAL));
    }

    for (size_t i = 0; i < mStorageImageDescriptors.size(); i++) {
      VkWriteDescriptorSet descriptorSet{};
      descriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorSet.dstSet          = dstSet;
      descriptorSet.dstBinding      = binding++;
      descriptorSet.dstArrayElement = 0;
      descriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorSet.descriptorCount = 1;
      descriptorSet.pImageInfo      = &storageImageInfos[i];

      descriptorWrites.push_back(descriptorSet);
    }

    std::vector<VkDescriptorBufferInfo> storageBufferInfos;
    for (size_t i = 0; i < mStorageBufferBundleDescriptors.size(); i++) {
      storageBufferInfos.push_back(mStorageBufferBundleDescriptors[i].data->getBuffer(j)->getDescriptorInfo());
    }

    for (size_t i = 0; i < mStorageBufferBundleDescriptors.size(); i++) {
      VkWriteDescriptorSet descriptorSet{};
      descriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorSet.dstSet          = dstSet;
      descriptorSet.dstBinding      = binding++;
      descriptorSet.dstArrayElement = 0;
      descriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorSet.descriptorCount = 1;
      descriptorSet.pBufferInfo     = &storageBufferInfos[i];

      descriptorWrites.push_back(descriptorSet);
    }

    vkUpdateDescriptorSets(vulkanApplicationContext.getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void Material::bind(VkCommandBuffer &commandBuffer, size_t currentFrame) {
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1,
                          &mDescriptorSets[currentFrame], 0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
}