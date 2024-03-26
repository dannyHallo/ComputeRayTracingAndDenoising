#pragma once

#include <string>

#ifdef PORTABLE_RESOURCES_FOLDER
static const std::string kRootDir = "./";
#else
static const std::string kRootDir = "C:/Users/danny/Desktop/voxel-lab/";
#endif // PORTABLE_RESOURCES_FOLDER

static const std::string kPathToResourceFolder        = kRootDir + "resources/";
static const std::string KPathToCompiledShadersFolder = kPathToResourceFolder + "compiled-shaders/";
