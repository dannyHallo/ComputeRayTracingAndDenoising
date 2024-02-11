#pragma once

#include "shaderc/shaderc.hpp"
#include "utils/file-io/ShaderFileReader.hpp"

#include <fstream>
#include <optional>
#include <string>
#include <vector>

class Logger;

class ShaderCompiler : public shaderc::Compiler {
public:
  ShaderCompiler(Logger *logger);

  std::optional<std::vector<char>> compileComputeShader(const std::string &sourceName,
                                                        std::string const &sourceCode);

private:
  Logger *_logger;
  shaderc::CompileOptions _defaultOptions;

}; // namespace ShaderCompiler