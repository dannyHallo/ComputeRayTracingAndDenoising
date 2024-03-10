#pragma once

#include <string>

// release mode, the resources folder is copied to the build folder
#ifdef NDEBUG
static const std::string kRootDir = "./";
#else
static const std::string kRootDir = "C:/Users/danny/Desktop/VoxelLab/";
#endif

static const std::string kPathToResourceFolder        = kRootDir + "resources/";
static const std::string KPathToCompiledShadersFolder = kPathToResourceFolder + "compiled-shaders/";
