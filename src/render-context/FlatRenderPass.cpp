#include "FlatRenderPass.h"
#include "app-context/VulkanApplicationContext.h"
#include "utils/vulkan.h"
#include "utils/systemLog.h"

#include <array>
#include <iostream>
#include <vector>

std::shared_ptr<VkRenderPass> FlatRenderPass::getBody() { return m_renderPass; }

std::shared_ptr<VkFramebuffer> FlatRenderPass::getFramebuffer(size_t index) { return m_swapChainFramebuffers[index]; }

// This shouldn't be called. Sorry for sloppy OOP.
std::shared_ptr<Image> FlatRenderPass::getColorImage() { return nullptr; }

FlatRenderPass::FlatRenderPass() {
  m_renderPass = std::make_shared<VkRenderPass>();
  for (size_t i = 0; i < VulkanGlobal::context.getSwapchainImages().size(); i++) {
    m_swapChainFramebuffers.push_back(std::make_shared<VkFramebuffer>());
  }
  createRenderPass();
  createFramebuffers();
}

FlatRenderPass::~FlatRenderPass() {
  std::cout << "Destroying flat pass"
            << "\n";
  for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++) {
    vkDestroyFramebuffer(VulkanGlobal::context.getDevice(), *m_swapChainFramebuffers[i], nullptr);
  }

  vkDestroyRenderPass(VulkanGlobal::context.getDevice(), *m_renderPass, nullptr);
}

void FlatRenderPass::createRenderPass() {
  // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes

  // attachment for a color buffer.
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format  = VulkanGlobal::context.getSwapchainImageFormat();
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  // load option: clear before render
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  // store option: rendered contents will be stored in memory and can be read later.
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  // irrelevant
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  // specifies which layout the image will have before the render pass
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  // specifies which layout the image will be after the render pass
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  // specifies which attachment to reference by its index in the attachment descriptions array
  // (VkAttachmentDescription).
  colorAttachmentRef.attachment = 0;
  // specifies which layout we would like the attachment to have during a subpass that uses this reference
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // the transition of the attachment will be like this:
  // VK_IMAGE_LAYOUT_UNDEFINED
  // subpass0
  // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  // subpass0 done
  // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR

  //   // Attachment for downsampling the final image.
  //   VkAttachmentDescription colorAttachmentResolve{};
  //   colorAttachmentResolve.format         = VulkanGlobal::context.getSwapchainImageFormat();
  //   colorAttachmentResolve.samples        = VK_SAMPLE_COUNT_1_BIT;
  //   colorAttachmentResolve.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  //   colorAttachmentResolve.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  //   colorAttachmentResolve.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  //   colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  //   colorAttachmentResolve.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  //   colorAttachmentResolve.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  //   VkAttachmentReference colorAttachmentResolveRef{};
  //   colorAttachmentResolveRef.attachment = 1;
  //   colorAttachmentResolveRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // the attchmentRef is for selecting the attachments to be used in a subpass from all attachments available in the
  // renderpass
  VkSubpassDescription subpass{};
  // compute subpass may also be supported in the future
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachmentRef;
  // subpass.pResolveAttachments = &colorAttachmentResolveRef;

  // a render pass may consist of several subpasses, each subpass may use several attachments

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments    = &colorAttachment;
  renderPassInfo.subpassCount    = 1;
  renderPassInfo.pSubpasses      = &subpass;

  // Subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass      = 0;
  dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // TODO: CANT UNDERSTAND
  dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass      = 0;
  dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  renderPassInfo.dependencyCount = 2;
  renderPassInfo.pDependencies   = dependencies.data();

  if (vkCreateRenderPass(VulkanGlobal::context.getDevice(), &renderPassInfo, nullptr, m_renderPass.get()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

void FlatRenderPass::createFramebuffers() {
  m_swapChainFramebuffers.resize(VulkanGlobal::context.getSwapchainImageViews().size());
  for (size_t i = 0; i < VulkanGlobal::context.getSwapchainImageViews().size(); i++) {
    std::array<VkImageView, 1> attachments = {VulkanGlobal::context.getSwapchainImageViews()[i]};
  
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = *m_renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments    = attachments.data();
    framebufferInfo.width           = VulkanGlobal::context.getSwapchainExtent().width;
    framebufferInfo.height          = VulkanGlobal::context.getSwapchainExtent().height;
    framebufferInfo.layers          = 1;

    if (vkCreateFramebuffer(VulkanGlobal::context.getDevice(), &framebufferInfo, nullptr, m_swapChainFramebuffers[i].get()) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}