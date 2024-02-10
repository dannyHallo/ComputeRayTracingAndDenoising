#include "ShaderFileWatchListener.hpp"

#include "utils/logger/Logger.hpp"

ShaderFileWatchListener::ShaderFileWatchListener(Logger *logger) : _logger(logger) {
  _fileWatcher = std::make_unique<efsw::FileWatcher>();
  _fileWatcher->addWatch(kPathToResourceFolder, this, true);
  _fileWatcher->watch();
}

void ShaderFileWatchListener::handleFileAction(efsw::WatchID /*watchid*/, const std::string &dir,
                                               const std::string &filename, efsw::Action action,
                                               std::string /*oldFilename*/) {
  if (action != efsw::Actions::Modified) {
    return;
  }
  _logger->print("shader file: dir{} filename{} has been modified! - reloading...", dir, filename);
}