#pragma once

// https://github.com/SpartanJ/efsw
#include "efsw/efsw.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

class Logger;
class PipelineScheduler;
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

  void handleFileAction(efsw::WatchID /*watchid*/, std::string const &dir,
                        std::string const &filename, efsw::Action action,
                        std::string oldFilename) override;

  void addWatchingPipeline(Pipeline *pipeline);

  // used to include headers
  void appendShaderFileToLastWatchedPipeline(std::string const &fullPathToShaderFile);

  void removeWatchingPipeline(Pipeline *pipeline);

private:
  Logger *_logger;
  std::unique_ptr<efsw::FileWatcher> _fileWatcher;

  std::unordered_map<std::string, std::unordered_set<Pipeline *>> _shaderFileNameToPipelines{};
  std::unordered_map<Pipeline *, std::unordered_set<std::string>> _pipelineToShaderFileNames{};
  std::unordered_set<Pipeline *> _pipelinesToRebuild{};

  Pipeline *_lastPipeline = nullptr;

  void _onRenderLoopBlocked();

  void _addWatchingFile(Pipeline *pipeline, std::string const &&fullPathToShaderFile);
};
