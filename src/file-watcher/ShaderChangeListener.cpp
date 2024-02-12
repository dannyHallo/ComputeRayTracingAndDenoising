#include "ShaderChangeListener.hpp"
#include "pipeline/Pipeline.hpp"
#include "scheduler/Scheduler.hpp"
#include "utils/config/RootDir.h"
#include "utils/event/EventType.hpp"
#include "utils/event/GlobalEventDispatcher.hpp"
#include "utils/logger/Logger.hpp"

#include <chrono>

ShaderChangeListener::ShaderChangeListener(Logger *logger)
    : _logger(logger), _fileWatcher(std::make_unique<efsw::FileWatcher>()) {
  _fileWatcher->addWatch(kRootDir + "src/shaders/", this, true);
  _fileWatcher->watch();

  GlobalEventDispatcher::get()
      .sink<E_RenderLoopBlocked>()
      .connect<&ShaderChangeListener::_onRenderLoopBlocked>(this);
}

ShaderChangeListener::~ShaderChangeListener() = default;

void ShaderChangeListener::handleFileAction(efsw::WatchID /*watchid*/, const std::string & /*dir*/,
                                            const std::string &filename, efsw::Action action,
                                            std::string /*oldFilename*/) {
  if (action != efsw::Actions::Modified) {
    return;
  }

  auto it = _watchingShaderFiles.find(filename);
  if (it == _watchingShaderFiles.end()) {
    return;
  }

  _logger->info("change of shader ({}) detected", filename);

  Pipeline *pipeline   = _shaderFileNameToPipeline[filename];
  Scheduler *scheduler = pipeline->getScheduler();

  _schedulerPipelinesToRebuild[scheduler].insert(pipeline);

  GlobalEventDispatcher::get().trigger<E_RenderLoopBlockRequest>();
}

void ShaderChangeListener::_onRenderLoopBlocked() {
  _logger->info("rebuild pipelines");

  // rebuild pipelines
  for (auto const &[scheduler, pipelines] : _schedulerPipelinesToRebuild) {
    for (auto *const pipeline : pipelines) {
      pipeline->build(false);
    }
  }

  // update schedulers
  for (auto const &[scheduler, pipelines] : _schedulerPipelinesToRebuild) {
    scheduler->update();
  }

  // then the render loop can continue
}

void ShaderChangeListener::addWatchingItem(Pipeline *pipeline) {
  auto const shaderFileName = pipeline->getShaderFileName();
  _logger->info("addWatchingItem: {}", shaderFileName);

  _watchingShaderFiles.insert(shaderFileName);

  if (_shaderFileNameToPipeline.find(shaderFileName) != _shaderFileNameToPipeline.end()) {
    _logger->error("shader file: {} called to be watched twice!", shaderFileName);
    exit(0);
  }

  _shaderFileNameToPipeline[shaderFileName] = pipeline;
}

void ShaderChangeListener::removeWatchingItem(Pipeline *pipeline) {
  auto const shaderFileName = pipeline->getShaderFileName();
  _logger->info("removeWatchingItem: {}", shaderFileName);

  _watchingShaderFiles.erase(shaderFileName);
  _shaderFileNameToPipeline.erase(shaderFileName);
}