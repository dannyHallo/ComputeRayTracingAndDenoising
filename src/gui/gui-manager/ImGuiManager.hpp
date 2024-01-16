#pragma once

#include "utils/incl/Vulkan.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class FpsGui;
class VulkanApplicationContext;
class Window;
class Logger;
class FpsSink;
class ImGuiManager {
public:
  ImGuiManager(VulkanApplicationContext *appContext, Window *window, Logger *logger,
               int framesInFlight);
  ~ImGuiManager();

  // delete copy and move
  ImGuiManager(const ImGuiManager &)            = delete;
  ImGuiManager &operator=(const ImGuiManager &) = delete;
  ImGuiManager(ImGuiManager &&)                 = delete;
  ImGuiManager &operator=(ImGuiManager &&)      = delete;

  void draw(FpsSink *fpsSink);

  [[nodiscard]] VkCommandBuffer getCommandBuffer(size_t currentFrame) {
    return _guiCommandBuffers[currentFrame];
  }

  void cleanupSwapchainDimensionRelatedResources();
  void createSwapchainDimensionRelatedResources();

  void recordGuiCommandBuffer(size_t currentFrame, uint32_t swapchainImageIndex);

private:
  VulkanApplicationContext *_appContext;
  Window *_window;
  Logger *_logger;

  std::unique_ptr<FpsGui> _fpsGui;

  VkDescriptorPool _guiDescriptorPool = VK_NULL_HANDLE;
  VkRenderPass _guiPass               = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> _guiFrameBuffers;
  std::vector<VkCommandBuffer> _guiCommandBuffers;

  void _initImgui();

  void _cleanupFrameBuffers();

  void _createGuiDescripterPool();
  void _createGuiCommandBuffers(int framesInFlight);
  void _createGuiRenderPass();
  void _createFramebuffers();

  void _syncMousePosition();

  void _drawConfigMenuItem();
  void _drawFpsMenuItem(double filteredFps, double fpsInTimeBucket);
};