#pragma once

#include <string>
#include <vector>

class Logger;
namespace ShaderFileReader {
std::string readShaderSourceCode(const std::string &fullPathToFile, Logger *logger);
std::vector<char> readShaderBinary(const std::string &fullPathToFile, Logger *logger);
}; // namespace ShaderFileReader
