#include "ShaderCompiler.hpp"

#include "CustomFileIncluder.hpp"
#include "utils/logger/Logger.hpp"

struct PathInfo {
  std::string fullPathToDir;
  std::string fileName;
};

namespace {
// input: a/b/c.glsl
// output: {a/b/, c.glsl}
PathInfo _getFullDirAndFileName(const std::string &fullPath, Logger *logger) {
  std::string dirName;
  std::string fileName;
  size_t found = fullPath.find_last_of('/');
  if (found != std::string::npos) {
    dirName  = fullPath.substr(0, found + 1);
    fileName = fullPath.substr(found + 1);
  } else {
    logger->error("failed to parse path: {}", fullPath);
  }
  return {dirName, fileName};
}
}; // namespace

ShaderCompiler::ShaderCompiler(Logger *logger) : _logger(logger) {
  std::unique_ptr<CustomFileIncluder> fileIncluder = std::make_unique<CustomFileIncluder>(logger);

  // _defaultOptions takes the ownership of fileIncluder, but doesn't provide a way to retrieve it,
  // so we need to store it as a raw pointer
  _fileIncluder = fileIncluder.get();

  _defaultOptions.SetIncluder(std::move(fileIncluder));
  // _defaultOptions.SetTargetSpirv(shaderc_spirv_version_1_3);
  _defaultOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
  _defaultOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
}

// TODO: but why this function needs to provide the file name?
std::optional<std::vector<uint32_t>>
ShaderCompiler::compileComputeShader(const std::string &fullPathToFile,
                                     std::string const &sourceCode) {
  // auto const fullPathToShaderCode = kRootDir + "src/shaders/" ...
  auto const fullDirAndFileName = _getFullDirAndFileName(fullPathToFile, _logger);

  _fileIncluder->setIncludeDir(fullDirAndFileName.fullPathToDir);

  std::optional<std::vector<uint32_t>> res = std::nullopt;
  shaderc_shader_kind const kind           = shaderc_glsl_compute_shader;

  shaderc::SpvCompilationResult compilationResult = this->CompileGlslToSpv(
      sourceCode, kind, fullDirAndFileName.fileName.c_str(), _defaultOptions);

  if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success) {
    _logger->warn(compilationResult.GetErrorMessage());
    return std::nullopt;
  }
  return std::vector<uint32_t>(compilationResult.cbegin(), compilationResult.cend());
}
