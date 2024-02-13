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

  // here, is some editors, (vscode, notepad++), when a file is saved, it will be saved twice, so
  // the block request is sent twice, however, when the render loop is blocked, the pipelines will
  // be rebuilt only once, a caching mechanism is used to avoid avoid duplicates
  // _logger->info("changes to {} is detected", filename);

  _pipelinesToRebuild.insert(_shaderFileNameToPipeline[filename]);

  // request to block the render loop, when the render loop is blocked, the pipelines will be
  // rebuilt
  GlobalEventDispatcher::get().trigger<E_RenderLoopBlockRequest>();
}

void ShaderChangeListener::_onRenderLoopBlocked() {
  // logging
  std::string pipelineNames;
  for (auto const &pipeline : _pipelinesToRebuild) {
    pipelineNames += pipeline->getShaderFileName() + " ";
  }
  _logger->info("rebuilding shaders due to changes: {}", pipelineNames);

  std::string rebuildFailedNames;

  std::unordered_set<Scheduler *> schedulersNeededToBeUpdated{};

  // rebuild pipelines
  for (auto const &pipeline : _pipelinesToRebuild) {
    // if shader module is rebuilt, then rebuild the pipeline, this is fail safe, because a valid
    // shader module is previously built and cached when initializing
    if (pipeline->buildAndCacheShaderModule(true)) {
      pipeline->build();
      schedulersNeededToBeUpdated.insert(pipeline->getScheduler());
      continue;
    }
    rebuildFailedNames += pipeline->getShaderFileName() + " ";
  }

  if (!rebuildFailedNames.empty()) {
    _logger->error("shaders building failed and are not cached: {}", rebuildFailedNames);
  }

  // update schedulers
  for (auto const &scheduler : schedulersNeededToBeUpdated) {
    scheduler->update();
  }

  // clear the cache
  _pipelinesToRebuild.clear();

  // then the render loop can continue
}

void ShaderChangeListener::addWatchingItem(Pipeline *pipeline) {
  auto const shaderFileName = pipeline->getShaderFileName();

  _watchingShaderFiles.insert(shaderFileName);

  if (_shaderFileNameToPipeline.find(shaderFileName) != _shaderFileNameToPipeline.end()) {
    _logger->error("shader file: {} called to be watched twice!", shaderFileName);
    exit(0);
  }

  _shaderFileNameToPipeline[shaderFileName] = pipeline;
}

void ShaderChangeListener::removeWatchingItem(Pipeline *pipeline) {
  auto const shaderFileName = pipeline->getShaderFileName();

  _watchingShaderFiles.erase(shaderFileName);
  _shaderFileNameToPipeline.erase(shaderFileName);
}