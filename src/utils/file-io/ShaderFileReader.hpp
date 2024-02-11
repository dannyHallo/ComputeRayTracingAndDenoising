#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class Logger;

namespace ShaderFileReader {
std::string readShaderSourceCode(const std::string &filename, Logger *logger);
std::vector<char> readShaderBinary(const std::string &filename, Logger *logger);
}; // namespace ShaderFileReader