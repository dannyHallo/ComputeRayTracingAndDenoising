#include "ShaderFileReader.hpp"

#include "utils/logger/Logger.hpp"

#include <fstream>

namespace ShaderFileReader {
std::string readShaderSourceCode(const std::string &filename, Logger *logger) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    logger->error("shader file: {} not found", filename);
    exit(0);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  return buffer.str();
}

std::vector<char> readShaderBinary(const std::string &filename, Logger *logger) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    logger->error("shader file: {} not found", filename);
    exit(0);
  }
  std::streamsize fileSize = static_cast<std::streamsize>(file.tellg());
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
  file.close();
  return buffer;
}

}; // namespace ShaderFileReader
