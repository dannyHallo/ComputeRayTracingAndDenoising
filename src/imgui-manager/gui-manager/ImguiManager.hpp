#pragma once

#include "volk.h"

#include <cstdint>
#include <memory>
#include <vector>

struct ConfigContainer;

class FpsGui;
class VulkanApplicationContext;
class Window;
class Logger;
class FpsSink;

class ImguiManager {
public:
  ImguiManager(VulkanApplicationContext *appContext, Window *window, Logger *logger,
               ConfigContainer *configContainer);
  ~ImguiManager();

  // delete copy and move
  ImguiManager(const ImguiManager &)            = delete;
  ImguiManager &operator=(const ImguiManager &) = delete;
  ImguiManager(ImguiManager &&)                 = delete;
  ImguiManager &operator=(ImguiManager &&)      = delete;

  void init();

  void draw(FpsSink *fpsSink);

  [[nodiscard]] VkCommandBuffer getCommandBuffer(size_t currentFrame) {
    return _guiCommandBuffers[currentFrame];
  }

  void onSwapchainResize();

  void recordCommandBuffer(size_t currentFrame, uint32_t imageIndex);

private:
  VulkanApplicationContext *_appContext;
  Window *_window;
  Logger *_logger;
  ConfigContainer *_configContainer;

  int _framesInFlight;
  bool _showFpsGraph = false;

  std::unique_ptr<FpsGui> _fpsGui;

  VkDescriptorPool _guiDescriptorPool = VK_NULL_HANDLE;
  VkRenderPass _guiPass               = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> _guiFrameBuffers;
  std::vector<VkCommandBuffer> _guiCommandBuffers;

  void _cleanupFrameBuffers();

  void _createGuiDescripterPool();
  void _createGuiCommandBuffers();
  void _createGuiRenderPass();
  void _createFramebuffers();

  void _syncMousePosition();

  void _drawConfigMenuItem();
  void _drawFpsMenuItem(double fpsInTimeBucket);
};