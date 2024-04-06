#include "ShaderChangeListener.hpp"

#include "scheduler/Scheduler.hpp"
#include "utils/config/RootDir.h"
#include "utils/event-dispatcher/GlobalEventDispatcher.hpp"
#include "utils/event-types/EventType.hpp"
#include "utils/logger/Logger.hpp"
#include "vulkan-wrapper/pipeline/Pipeline.hpp"

namespace {
// input a/b/\c/d.xxx
// output a/b/c/d.xxx
std::string _normalizePath(const std::string &path) {
  std::string normalizedPath = path;

  // replace backslashes with forward slashes
  std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

  // remove any double slashes
  std::string::size_type pos = 0;
  while ((pos = normalizedPath.find("//", pos)) != std::string::npos) {
    normalizedPath.erase(pos, 1);
    // Do not advance pos to ensure we check the new position in case of consecutive slashes
  }

  return normalizedPath;
}
}; // namespace

ShaderChangeListener::ShaderChangeListener(Logger *logger)
    : _logger(logger), _fileWatcher(std::make_unique<efsw::FileWatcher>()) {
  _fileWatcher->addWatch(kPathToResourceFolder + "shaders/", this, true);
  _fileWatcher->watch();

  GlobalEventDispatcher::get()
      .sink<E_RenderLoopBlocked>()
      .connect<&ShaderChangeListener::_onRenderLoopBlocked>(this);
}

ShaderChangeListener::~ShaderChangeListener() { GlobalEventDispatcher::get().disconnect(this); }

void ShaderChangeListener::handleFileAction(efsw::WatchID /*watchid*/, const std::string &dir,
                                            const std::string &filename, efsw::Action action,
                                            std::string /*oldFilename*/) {
  if (action != efsw::Actions::Modified) {
    return;
  }

  std::string normalizedPathToFile = _normalizePath(dir + '/' + filename);

  auto it = _watchingShaderFiles.find(normalizedPathToFile);
  if (it == _watchingShaderFiles.end()) {
    return;
  }

  // here, is some editors, (vscode, notepad++), when a file is saved, it will be saved twice, so
  // the block request is sent twice, however, when the render loop is blocked, the pipelines will
  // be rebuilt only once, a caching mechanism is used to avoid avoid duplicates

  _pipelinesToRebuild.insert(_shaderFileNameToPipeline[normalizedPathToFile]);

  // request to block the render loop, when the render loop is blocked, the pipelines will be
  // rebuilt
  GlobalEventDispatcher::get().trigger<E_RenderLoopBlockRequest>(
      E_RenderLoopBlockRequest{it->second});
}

void ShaderChangeListener::_onRenderLoopBlocked() {
  // logging
  std::string pipelineNames;
  for (auto const &pipeline : _pipelinesToRebuild) {
    pipelineNames += pipeline->getFullPathToShaderSourceCode() + " ";
  }
  _logger->info("rebuilding shaders due to changes: {}", pipelineNames);

  std::string rebuildFailedNames;

  std::unordered_set<Scheduler *> schedulersNeededToBeUpdated{};

  // rebuild pipelines
  for (auto const &pipeline : _pipelinesToRebuild) {
    // if shader module is rebuilt, then rebuild the pipeline, this is fail safe, because a valid
    // shader module is previously built and cached when initializing
    if (pipeline->compileAndCacheShaderModule(true)) {
      pipeline->build();
      schedulersNeededToBeUpdated.insert(pipeline->getScheduler());
      continue;
    }
    rebuildFailedNames += pipeline->getFullPathToShaderSourceCode() + " ";
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

  // then the render loop can be continued
}

void ShaderChangeListener::addWatchingItem(Pipeline *pipeline, bool needToRebuildSvo) {
  auto const fullPathToFile = pipeline->getFullPathToShaderSourceCode();

  _logger->info("file added to change watch list: {}", fullPathToFile);
  _watchingShaderFiles[fullPathToFile] = needToRebuildSvo;

  if (_shaderFileNameToPipeline.find(fullPathToFile) != _shaderFileNameToPipeline.end()) {
    _logger->error("shader file: {} called to be watched twice!", fullPathToFile);
    exit(0);
  }

  _shaderFileNameToPipeline[fullPathToFile] = pipeline;
}

void ShaderChangeListener::removeWatchingItem(Pipeline *pipeline) {
  auto const fullPathToFile = pipeline->getFullPathToShaderSourceCode();

  _watchingShaderFiles.erase(fullPathToFile);
  _shaderFileNameToPipeline.erase(fullPathToFile);
}