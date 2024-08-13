#include "ShaderChangeListener.hpp"

#include "application/BlockState.hpp"
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
    // do not advance pos to ensure we check the new position in case of consecutive slashes
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

void ShaderChangeListener::handleFileAction(efsw::WatchID /*watchid*/, std::string const &dir,
                                            std::string const &filename, efsw::Action action,
                                            std::string /*oldFilename*/) {
  if (action != efsw::Actions::Modified) {
    return;
  }

  std::string normalizedPathToFile = _normalizePath(dir + '/' + filename);

  auto it = _shaderFileNameToPipelines.find(normalizedPathToFile);
  if (it == _shaderFileNameToPipelines.end()) {
    return;
  }

  // here, is some editors, (vscode, notepad++), when a file is saved, it might be saved twice
  // simultaneously, so the block request is sent twice, however, when the render loop is blocked,
  // the pipelines will be rebuilt only once, a caching mechanism is used to avoid duplicates
  _pipelinesToRebuild.insert(it->second.begin(), it->second.end());

  // request to block the render loop, when the render loop is blocked, the pipelines will be
  // rebuilt
  uint32_t blockStateBits = BlockState::kShaderChanged;
  GlobalEventDispatcher::get().trigger<E_RenderLoopBlockRequest>(
      E_RenderLoopBlockRequest{blockStateBits});
}

void ShaderChangeListener::_onRenderLoopBlocked() {
  // logging
  std::string pipelineNames;
  for (auto const &pipeline : _pipelinesToRebuild) {
    pipelineNames += pipeline->getFullPathToShaderSourceCode() + " ";
  }
  _logger->info("rebuilding shaders due to changes: {}", pipelineNames);

  std::string rebuildFailedNames;

  std::unordered_set<PipelineScheduler *> schedulersNeededToBeUpdated{};

  // rebuild pipelines
  for (auto const &pipeline : _pipelinesToRebuild) {
    // if shader module is rebuilt, then rebuild the pipeline, this is fail safe, because a valid
    // shader module is previously built and cached when initializing
    if (pipeline->compileAndCacheShaderModule()) {
      pipeline->build();
      schedulersNeededToBeUpdated.insert(pipeline->getScheduler());
      continue;
    }
    rebuildFailedNames += pipeline->getFullPathToShaderSourceCode() + " ";
  }

  if (!rebuildFailedNames.empty()) {
    _logger->error("shaders building failed and are not cached: {}", rebuildFailedNames);
  }

  // update affected schedulers
  for (auto const &scheduler : schedulersNeededToBeUpdated) {
    scheduler->onPipelineRebuilt();
  }

  // clear the cache
  _pipelinesToRebuild.clear();

  // then the render loop can be continued
}

void ShaderChangeListener::_addWatchingFile(Pipeline *pipeline,
                                            std::string const &&fullPathToShaderFile) {
  auto it = _shaderFileNameToPipelines.find(fullPathToShaderFile);
  if (it == _shaderFileNameToPipelines.end()) {
    _shaderFileNameToPipelines[fullPathToShaderFile] = std::unordered_set<Pipeline *>{};
  }

  auto it2 = _pipelineToShaderFileNames.find(pipeline);
  if (it2 == _pipelineToShaderFileNames.end()) {
    _pipelineToShaderFileNames[pipeline] = std::unordered_set<std::string>{};
  }

  _shaderFileNameToPipelines[fullPathToShaderFile].insert(pipeline);
  _pipelineToShaderFileNames[pipeline].insert(fullPathToShaderFile);

  _logger->info("file added to change watch list: {}", fullPathToShaderFile);
}

void ShaderChangeListener::addWatchingPipeline(Pipeline *pipeline) {
  _addWatchingFile(pipeline, pipeline->getFullPathToShaderSourceCode());
}

void ShaderChangeListener::appendShaderFileToLastWatchedPipeline(
    std::string const &fullPathToShaderFile) {
  _addWatchingFile(_lastPipeline, std::move(fullPathToShaderFile));
}

void ShaderChangeListener::removeWatchingPipeline(Pipeline *pipeline) {
  auto it = _pipelineToShaderFileNames.find(pipeline);
  assert(it != _pipelineToShaderFileNames.end());
  auto const &associatedShaderFileNames = it->second;
  for (auto const &fn : associatedShaderFileNames) {
    auto it2 = _shaderFileNameToPipelines.find(fn);
    assert(it2 != _shaderFileNameToPipelines.end());
    auto &pls = it2->second;
    auto it3  = pls.find(pipeline);
    assert(it3 != pls.end());
    pls.erase(it3);
  }
  _pipelineToShaderFileNames.erase(pipeline);
}
