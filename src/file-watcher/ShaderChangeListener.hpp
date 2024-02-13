#pragma once

// https://github.com/SpartanJ/efsw
#include "efsw/efsw.hpp"

#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>

class Logger;
class Scheduler;
class Pipeline;

class ShaderChangeListener : public efsw::FileWatchListener {
public:
  ShaderChangeListener(Logger *logger);
  ~ShaderChangeListener() override;

  // disable copy and move
  ShaderChangeListener(ShaderChangeListener const &)            = delete;
  ShaderChangeListener(ShaderChangeListener &&)                 = delete;
  ShaderChangeListener &operator=(ShaderChangeListener const &) = delete;
  ShaderChangeListener &operator=(ShaderChangeListener &&)      = delete;

  void handleFileAction(efsw::WatchID /*watchid*/, const std::string &dir,
                        const std::string &filename, efsw::Action action,
                        std::string oldFilename) override;

  void addWatchingItem(Pipeline *pipeline, bool needToRebuildSvo = false);
  void removeWatchingItem(Pipeline *pipeline);

private:
  Logger *_logger;

  // carries if svo needs to be rebuilt
  std::unordered_map<std::string, bool> _watchingShaderFiles;

  std::unordered_map<std::string, Pipeline *> _shaderFileNameToPipeline;
  std::unordered_set<Pipeline *> _pipelinesToRebuild;

  std::unique_ptr<efsw::FileWatcher> _fileWatcher;

  void _onRenderLoopBlocked();
};
