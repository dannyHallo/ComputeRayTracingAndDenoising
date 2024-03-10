#pragma once

#include <string>

// release mode, the resources folder is copied to the build folder
static const std::string kRootDir = "./";

static const std::string kPathToResourceFolder        = kRootDir + "resources/";
static const std::string KPathToCompiledShadersFolder = kPathToResourceFolder + "compiled-shaders/";
