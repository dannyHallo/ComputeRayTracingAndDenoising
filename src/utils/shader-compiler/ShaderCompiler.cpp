#include "ShaderCompiler.hpp"

#include "utils/logger/Logger.hpp"

ShaderCompiler::ShaderCompiler(Logger *logger) : _logger(logger) {
  _defaultOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
}

std::optional<std::vector<char>> ShaderCompiler::compileShader(const std::string &sourceName,
                                                               shaderc_shader_kind kind,
                                                               std::vector<char> const &source) {
  std::optional<std::vector<char>> res = std::nullopt;

  char const *sourceData = source.data();
  shaderc::SpvCompilationResult compilationResult =
      this->CompileGlslToSpv(sourceData, source.size(), kind, sourceName.c_str(), _defaultOptions);

  if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success) {
    _logger->warn(compilationResult.GetErrorMessage());
    return std::nullopt;
  }

  res = std::vector<char>(compilationResult.cbegin(), compilationResult.cend());
  return res;
}