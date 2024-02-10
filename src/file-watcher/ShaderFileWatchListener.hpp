#pragma once

// https://github.com/SpartanJ/efsw
#include "efsw/efsw.hpp"
#include "utils/config/RootDir.h"

#include <iostream>
#include <memory>

class Logger;

class ShaderFileWatchListener : public efsw::FileWatchListener {
public:
  ShaderFileWatchListener(Logger *logger);

  void handleFileAction(efsw::WatchID /*watchid*/, const std::string &dir,
                        const std::string &filename, efsw::Action action,
                        std::string oldFilename) override;

private:
  Logger *_logger;

  std::unique_ptr<efsw::FileWatcher> _fileWatcher;
};
