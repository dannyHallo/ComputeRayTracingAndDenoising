#pragma once

#include "shaderc/shaderc.hpp"
#include "utils/file-io/ShaderFileReader.hpp"

#include <fstream>
#include <optional>
#include <vector>

class Logger;

class ShaderCompiler : public shaderc::Compiler {
public:
  ShaderCompiler(Logger *logger);

private:
  Logger *_logger;
  shaderc::CompileOptions _defaultOptions;

  std::optional<std::vector<char>> compileShader(const std::string &sourceName,
                                                 shaderc_shader_kind kind,
                                                 std::vector<char> const &source);
}; // namespace ShaderCompiler