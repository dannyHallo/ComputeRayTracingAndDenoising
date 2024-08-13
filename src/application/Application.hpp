#pragma once

#include "app-context/VulkanApplicationContext.hpp"

#include "utils/event-types/EventType.hpp"
#include "utils/logger/Logger.hpp"
#include "window/KeyboardInfo.hpp"

#include <memory>
#include <vector>

struct ConfigContainer;

class Logger;
class Window;
class SvoBuilder;
class SvoTracer;
class ImguiManager;
class Camera;
class ShadowMapCamera;
class FpsSink;
class ShaderCompiler;
class ShaderChangeListener;

class Application {
public:
  Application(Logger *logger);
  ~Application();

  // disable move and copy
  Application(const Application &)            = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&)                 = delete;
  Application &operator=(Application &&)      = delete;

  void run();

private:
  Logger *_logger;

  std::unique_ptr<VulkanApplicationContext> _appContext          = nullptr;
  std::unique_ptr<ConfigContainer> _configContainer              = nullptr;
  std::unique_ptr<ShaderChangeListener> _shaderFileWatchListener = nullptr;
  std::unique_ptr<ShaderCompiler> _shaderCompiler                = nullptr;
  std::unique_ptr<Window> _window                                = nullptr;
  std::unique_ptr<SvoBuilder> _svoBuilder                        = nullptr;
  std::unique_ptr<SvoTracer> _svoTracer                          = nullptr;
  std::unique_ptr<ImguiManager> _imguiManager                    = nullptr;
  std::unique_ptr<FpsSink> _fpsSink                              = nullptr;

  // semaphores and fences for synchronization
  std::vector<VkSemaphore> _imageAvailableSemaphores{};
  std::vector<VkSemaphore> _renderFinishedSemaphores{};
  std::vector<VkFence> _framesInFlightFences{};

  // BlockState _blockState = BlockState::kUnblocked;
  uint32_t _blockStateBits = 0;

  void _applicationKeyboardCallback(KeyboardInfo const &keyboardInfo);

  void _createSemaphoresAndFences();
  void _onSwapchainResize();
  void _waitForTheWindowToBeResumed();
  void _drawFrame();
  void _mainLoop();
  void _init();
  void _cleanup();

  void _onRenderLoopBlockRequest(E_RenderLoopBlockRequest const &event);
  void _buildScene();
};