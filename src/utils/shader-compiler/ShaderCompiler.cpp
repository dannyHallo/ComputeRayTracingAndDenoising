#include "ShaderCompiler.hpp"

#include "CustomFileIncluder.hpp"
#include "utils/logger/Logger.hpp"

ShaderCompiler::ShaderCompiler(Logger *logger) : _logger(logger) {
  std::unique_ptr<CustomFileIncluder> fileIncluder = std::make_unique<CustomFileIncluder>(logger);
  _defaultOptions.SetIncluder(std::move(fileIncluder));
  _defaultOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
}

std::optional<std::vector<char>>
ShaderCompiler::compileComputeShader(const std::string &sourceName, std::string const &sourceCode) {
  std::optional<std::vector<char>> res = std::nullopt;
  shaderc_shader_kind const kind       = shaderc_glsl_compute_shader;

  shaderc::SpvCompilationResult compilationResult =
      this->CompileGlslToSpv(sourceCode, kind, sourceName.c_str(), _defaultOptions);

  if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success) {
    _logger->warn(compilationResult.GetErrorMessage());
    return std::nullopt;
  }

  res = std::vector<char>(compilationResult.cbegin(), compilationResult.cend());
  return res;
}