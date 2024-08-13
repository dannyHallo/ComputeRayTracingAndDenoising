#pragma once

#include "shaderc/shaderc.hpp"
#include "utils/io/ShaderFileReader.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

class Logger;
class CustomFileIncluder;

class ShaderCompiler : public shaderc::Compiler {
public:
  ShaderCompiler(Logger *logger,
                 std::function<void(std::string const &)> includeCallback = nullptr);

  std::optional<std::vector<uint32_t>> compileComputeShader(const std::string &fullPathToFile,
                                                            std::string const &sourceCode);

private:
  Logger *_logger;
  shaderc::CompileOptions _defaultOptions;
  CustomFileIncluder *_fileIncluder;

}; // namespace ShaderCompiler