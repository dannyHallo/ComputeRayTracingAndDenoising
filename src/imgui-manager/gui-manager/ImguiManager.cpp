#include "ImguiManager.hpp"

#include "imgui.h"
#include "implot.h"

#include "../gui-elements/FpsGui.hpp"
#include "../imgui-backends/imgui_impl_glfw.h"
#include "../imgui-backends/imgui_impl_vulkan.h"
#include "app-context/VulkanApplicationContext.hpp"
#include "application/svo-tracer/SvoTracerTweakingData.hpp"
#include "utils/color-palette/ColorPalette.hpp"
#include "utils/config/RootDir.h"
#include "utils/fps-sink/FpsSink.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/toml-config/TomlConfigReader.hpp"
#include "window/Window.hpp"

ImguiManager::ImguiManager(VulkanApplicationContext *appContext, Window *window, Logger *logger,
                           TomlConfigReader *tomlConfigReader, int framesInFlight,
                           SvoTracerTweakingData *SvoTracerDataGpu)
    : _appContext(appContext), _window(window), _logger(logger),
      _tomlConfigReader(tomlConfigReader), _framesInFlight(framesInFlight),
      _svoTracerTweakingData(SvoTracerDataGpu), _colorPalette(std::make_unique<ColorPalette>()) {
  _loadConfig();
}

ImguiManager::~ImguiManager() {

  for (auto &guiCommandBuffer : _guiCommandBuffers) {
    vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getGuiCommandPool(), 1,
                         &guiCommandBuffer);
  }

  vkDestroyRenderPass(_appContext->getDevice(), _guiPass, nullptr);

  _cleanupFrameBuffers();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();

  vkDestroyDescriptorPool(_appContext->getDevice(), _guiDescriptorPool, nullptr);

  ImPlot::DestroyContext();
  ImGui::DestroyContext();
}

void ImguiManager::_loadConfig() {
  _fontSize = _tomlConfigReader->getConfig<float>("ImguiManager.fontSize");
}

void ImguiManager::_buildColorPalette() {
  // https://colorhunt.co/palette/1d2b537e2553ff004dfaef5d
  _colorPalette->addColor("DarkBlue", Color(29, 43, 83));
  _colorPalette->addColor("DarkPurple", Color(126, 37, 83));
  _colorPalette->addColor("LightRed", Color(255, 0, 77));
  _colorPalette->addColor("LightYellow", Color(250, 239, 39)); // NOLINT
}

void ImguiManager::_cleanupFrameBuffers() {
  for (auto &guiFrameBuffer : _guiFrameBuffers) {
    vkDestroyFramebuffer(_appContext->getDevice(), guiFrameBuffer, nullptr);
  }
}

void ImguiManager::onSwapchainResize() {
  _cleanupFrameBuffers();
  _createFramebuffers();
}

void ImguiManager::init() {
  _buildColorPalette();

  _fpsGui = std::make_unique<FpsGui>(_colorPalette.get());

  _createGuiCommandBuffers();
  _createGuiRenderPass();
  _createFramebuffers();
  _createGuiDescripterPool();

  // setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF((kPathToResourceFolder + "/fonts/editundo/editundo.ttf").c_str(),
                               _fontSize);

  io.ConfigFlags |= ImGuiWindowFlags_NoNavInputs;

  _setImguiPalette();

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForVulkan(_window->getGlWindow(), true);

  ImGui_ImplVulkan_InitInfo info = {};
  info.Instance                  = _appContext->getVkInstance();
  info.PhysicalDevice            = _appContext->getPhysicalDevice();
  info.Device                    = _appContext->getDevice();
  info.QueueFamily               = _appContext->getQueueFamilyIndices().graphicsFamily;
  info.Queue                     = _appContext->getGraphicsQueue();
  info.PipelineCache             = VK_NULL_HANDLE;
  info.DescriptorPool            = _guiDescriptorPool;
  info.RenderPass                = _guiPass;
  info.Allocator                 = VK_NULL_HANDLE;
  info.MinImageCount             = static_cast<uint32_t>(_appContext->getSwapchainImagesCount());
  info.ImageCount                = static_cast<uint32_t>(_appContext->getSwapchainImagesCount());
  info.CheckVkResultFn           = nullptr;
  if (!ImGui_ImplVulkan_Init(&info)) {
    _logger->info("failed to init impl");
  }
}

void ImguiManager::_createGuiDescripterPool() {
  int constexpr kMaxDescriptorCount           = 1000;
  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, kMaxDescriptorCount},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, kMaxDescriptorCount},
  };

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  // this descriptor pool is created only for once, so we can set the flag to allow individual
  // descriptor sets to be de-allocated
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  // imgui actually uses only 1 descriptor set
  poolInfo.maxSets       = static_cast<uint32_t>(kMaxDescriptorCount * poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());

  vkCreateDescriptorPool(_appContext->getDevice(), &poolInfo, nullptr, &_guiDescriptorPool);
}

void ImguiManager::_createGuiCommandBuffers() {
  _guiCommandBuffers.resize(_framesInFlight);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = _appContext->getGuiCommandPool();
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)_guiCommandBuffers.size();

  vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, _guiCommandBuffers.data());
}

void ImguiManager::_createGuiRenderPass() {
  // Imgui Pass, right after the main pass
  VkAttachmentDescription attachment = {};
  attachment.format                  = _appContext->getSwapchainImageFormat();
  attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
  // Load onto the current render pass
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  // Store img until display time
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  // No stencil
  attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  // Present image right after this pass
  attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachment = {};
  colorAttachment.attachment            = 0;
  colorAttachment.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachment;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount        = 1;
  renderPassCreateInfo.pAttachments           = &attachment;
  renderPassCreateInfo.subpassCount           = 1;
  renderPassCreateInfo.pSubpasses             = &subpass;
  renderPassCreateInfo.dependencyCount        = 1;
  renderPassCreateInfo.pDependencies          = &dependency;

  vkCreateRenderPass(_appContext->getDevice(), &renderPassCreateInfo, nullptr, &_guiPass);
}

void ImguiManager::_createFramebuffers() {
  // Create gui frame buffers for gui pass to use
  // Each frame buffer will have an attachment of VkImageView, in this case, the
  // attachments are mSwapchainImageViews
  _guiFrameBuffers.resize(_appContext->getSwapchainImagesCount());

  uint32_t const w = _appContext->getSwapchainExtentWidth();
  uint32_t const h = _appContext->getSwapchainExtentHeight();

  // Iterate through image views
  for (size_t i = 0; i < _appContext->getSwapchainImagesCount(); i++) {
    VkImageView attachment = _appContext->getSwapchainImageViews()[i];

    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass      = _guiPass;
    frameBufferCreateInfo.attachmentCount = 1;
    frameBufferCreateInfo.pAttachments    = &attachment;
    frameBufferCreateInfo.width           = w;
    frameBufferCreateInfo.height          = h;
    frameBufferCreateInfo.layers          = 1;

    vkCreateFramebuffer(_appContext->getDevice(), &frameBufferCreateInfo, nullptr,
                        &_guiFrameBuffers[i]);
  }
}

void ImguiManager::recordCommandBuffer(size_t currentFrame, uint32_t imageIndex) {
  VkCommandBuffer commandBuffer = _guiCommandBuffers[currentFrame];

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags            = 0;       // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  // a call to vkBeginCommandBuffer will implicitly reset the command buffer
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass            = _guiPass;
  renderPassInfo.framebuffer           = _guiFrameBuffers[imageIndex];
  renderPassInfo.renderArea.extent     = _appContext->getSwapchainExtent();

  VkClearValue clearValue{};
  clearValue.color = {{0.0F, 0.0F, 0.0F, 1.0F}};

  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues    = &clearValue;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Record Imgui Draw Data and draw funcs into command buffer
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  vkCmdEndRenderPass(commandBuffer);
  vkEndCommandBuffer(commandBuffer);
}

void ImguiManager::_drawConfigMenuItem() {
  if (ImGui::BeginMenu("Config")) {
    ImGui::SeparatorText("Debug");

    ImGui::Checkbox("Debug B1", &_svoTracerTweakingData->debugB1);
    ImGui::SliderFloat("Debug F1", &_svoTracerTweakingData->debugF1, 0.0F, 1.0F);
    ImGui::SliderInt("Debug I1", &_svoTracerTweakingData->debugI1, 0, 10);

    ///

    ImGui::SeparatorText("Atmos");
    ImGui::SliderFloat("Sun Altitude", &_svoTracerTweakingData->sunAltitude, 0.F, 180.F);
    ImGui::SliderFloat("Sun Azimuth", &_svoTracerTweakingData->sunAzimuth, -180.F, 180.F);
    ImGui::InputFloat3("Rayleigh Scattering Base",
                       &_svoTracerTweakingData->rayleighScatteringBase.x);
    ImGui::SliderFloat("Mie Scattering Base", &_svoTracerTweakingData->mieScatteringBase, 0.0F,
                       10.0F);
    ImGui::SliderFloat("Mie Absorption Base", &_svoTracerTweakingData->mieAbsorptionBase, 0.0F,
                       10.0F);
    ImGui::InputFloat3("Ozone Absorption Base", &_svoTracerTweakingData->ozoneAbsorptionBase.x);
    ImGui::SliderFloat("Sun Luminance", &_svoTracerTweakingData->sunLuminance, 0.0F, 10.0F);
    ImGui::SliderFloat("Atmos Luminance", &_svoTracerTweakingData->atmosLuminance, 0.0F, 10.0F);
    ImGui::SliderFloat("Sun Size", &_svoTracerTweakingData->sunSize, 0.0F, 100.0F);

    ///

    ImGui::SeparatorText("Tracing");
    ImGui::Checkbox("Visualize Chunks", &_svoTracerTweakingData->visualizeChunks);
    ImGui::Checkbox("Visualize Octree", &_svoTracerTweakingData->visualizeOctree);
    ImGui::Checkbox("Beam Optimization", &_svoTracerTweakingData->beamOptimization);
    ImGui::Checkbox("Trace Indirect Ray", &_svoTracerTweakingData->traceIndirectRay);

    ///

    ImGui::SeparatorText("Filtering");
    ImGui::Checkbox("TAA", &_svoTracerTweakingData->taa);
    ImGui::SliderFloat("Temporal Alpha", &_svoTracerTweakingData->temporalAlpha, 0.0F, 1.0F);
    ImGui::SliderInt("A-Trous Iteration Count", &_svoTracerTweakingData->aTrousIterationCount, 0,
                     5);
    ImGui::SliderFloat("Phi Z - Far End", &_svoTracerTweakingData->minPhiZ, 0.0F, 1.0F);
    ImGui::SliderFloat("Phi Z - Near End", &_svoTracerTweakingData->maxPhiZ, 0.0F, 1.0F);

    ///

    ImGui::SeparatorText("Post Processing");
    ImGui::SliderFloat("Explosure", &_svoTracerTweakingData->explosure, 0.0F, 20.0F);

    ///

    ImGui::EndMenu();
  }
}

void ImguiManager::_drawFpsMenuItem(double fpsInTimeBucket) {
  std::string const kFpsString = std::to_string(static_cast<int>(fpsInTimeBucket)) + " FPS";

  // calculate the right-aligned position for the FPS menu
  auto windowWidth      = ImGui::GetWindowContentRegionMax().x;
  auto fpsMenuWidth     = ImGui::CalcTextSize(kFpsString.c_str()).x;
  auto rightAlignedPosX = windowWidth - fpsMenuWidth;

  // set the cursor position to the calculated position
  ImGui::SetCursorPosX(rightAlignedPosX);
  ImGui::SetNextItemWidth(fpsMenuWidth);
  if (ImGui::BeginMenu("##FpsMenu")) {
    ImGui::Checkbox("Show Fps", &_showFpsGraph);
    ImGui::EndMenu();
  }

  ImGui::SetCursorPosX(rightAlignedPosX);

  ImGui::Text("%s", kFpsString.c_str());
}

void ImguiManager::_syncMousePosition() {
  auto &io = ImGui::GetIO();

  // TODO: query for the info directly from the window class
  // the mousePos is not synced correctly when the window is not focused
  // so we set it manually here
  io.MousePos = ImVec2(static_cast<float>(_window->getCursorXPos()),
                       static_cast<float>(_window->getCursorYPos()));
}

void ImguiManager::_setImguiPalette() {
  auto const &darkBlue   = _colorPalette->getColorByName("DarkBlue");
  auto const &darkPurple = _colorPalette->getColorByName("DarkPurple");

  ImGuiStyle &style                = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text]      = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
  style.Colors[ImGuiCol_MenuBarBg] = darkPurple.getImVec4();
  style.Colors[ImGuiCol_PopupBg]   = darkBlue.getImVec4();
}

void ImguiManager::draw(FpsSink *fpsSink) {
  double const filteredFps     = fpsSink->getFilteredFps();
  double const fpsInTimeBucket = fpsSink->getFpsInTimeBucket();

  _syncMousePosition();

  ImGui_ImplVulkan_NewFrame();
  // handles the user input, and the resizing of the window
  ImGui_ImplGlfw_NewFrame();

  ImGui::NewFrame();

  ImGui::BeginMainMenuBar();
  _drawConfigMenuItem();
  _drawFpsMenuItem(fpsInTimeBucket);
  ImGui::EndMainMenuBar();

  if (_showFpsGraph) {
    _fpsGui->update(_appContext, filteredFps);
  }

  ImGui::Render();
}
