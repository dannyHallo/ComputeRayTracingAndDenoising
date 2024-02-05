#pragma once

#include "utils/incl/Vulkan.hpp"

#include <cstdint>
#include <memory>
#include <vector>

struct SvoTracerTweakingData;

class FpsGui;
class VulkanApplicationContext;
class Window;
class Logger;
class FpsSink;
class ColorPalette;
class ImguiManager {
public:
  ImguiManager(VulkanApplicationContext *appContext, Window *window, Logger *logger,
               int framesInFlight, SvoTracerTweakingData *SvoTracerDataGpu);
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
  int _framesInFlight;
  bool _showFpsGraph = true;

  SvoTracerTweakingData *_svoTracerTweakingData;

  std::unique_ptr<FpsGui> _fpsGui;
  std::unique_ptr<ColorPalette> _colorPalette;

  VkDescriptorPool _guiDescriptorPool = VK_NULL_HANDLE;
  VkRenderPass _guiPass               = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> _guiFrameBuffers;
  std::vector<VkCommandBuffer> _guiCommandBuffers;

  void _cleanupFrameBuffers();

  void _buildColorPalette();
  void _setImguiPalette();

  void _createGuiDescripterPool();
  void _createGuiCommandBuffers(int framesInFlight);
  void _createGuiRenderPass();
  void _createFramebuffers();

  void _syncMousePosition();

  void _drawConfigMenuItem();
  void _drawFpsMenuItem(double fpsInTimeBucket);
};