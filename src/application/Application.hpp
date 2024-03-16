#pragma once

#include "app-context/VulkanApplicationContext.hpp"

#include "utils/event/EventType.hpp"
#include "utils/logger/Logger.hpp"

#include <memory>
#include <vector>

class Logger;
class Window;
class SvoBuilder;
class SvoTracer;
class ImguiManager;
class Camera;
class FpsSink;
class ShaderCompiler;
class ShaderChangeListener;
class TomlConfigReader;

enum class BlockState {
  kUnblocked,
  kBlocked,
  kBlockedAndNeedToRebuildSvo,
};

class Application {
public:
  Application(Logger *logger);
  ~Application();

  // disable move and copy
  Application(const Application &)            = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&)                 = delete;
  Application &operator=(Application &&)      = delete;

  Camera *getCamera();
  void run();

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  // config
  uint32_t _framesInFlight{};
  void _loadConfig();

  std::unique_ptr<FpsSink> _fpsSink;
  std::unique_ptr<ImguiManager> _imguiManager;
  std::unique_ptr<Camera> _camera;
  std::unique_ptr<Window> _window;
  std::unique_ptr<ShaderCompiler> _shaderCompiler;
  std::unique_ptr<ShaderChangeListener> _shaderFileWatchListener;
  std::unique_ptr<TomlConfigReader> _tomlConfigReader;

  std::unique_ptr<SvoBuilder> _svoBuilder;
  std::unique_ptr<SvoTracer> _svoTracer;

  // semaphores and fences for synchronization
  std::vector<VkSemaphore> _imageAvailableSemaphores;
  std::vector<VkSemaphore> _renderFinishedSemaphores;
  std::vector<VkFence> _framesInFlightFences;

  BlockState _blockState = BlockState::kUnblocked;

  void _createSemaphoresAndFences();
  void _onSwapchainResize();
  void _waitForTheWindowToBeResumed();
  void _drawFrame();
  void _mainLoop();
  void _init();
  void _cleanup();
  bool _needToToggleWindowStyle();

  void _onRenderLoopBlockRequest(E_RenderLoopBlockRequest const &event);
  void _buildScene();
};