#pragma once

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

namespace Utils {

static std::optional<std::string> readText(std::string const &filename) {
  std::ifstream stream(filename);

  if (!stream) {
    std::cerr << "Failed to open " << filename << std::endl;
    return std::nullopt;
  }

  std::stringstream ss;
  while (stream) {
    std::string line;
    std::getline(stream, line);
    ss << line;
  }
  return ss.str();
}

static std::optional<std::vector<char>>
readByteCode(std::string const &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file) {
    std::cerr << "Failed to open " << filename << std::endl;
    return std::nullopt;
  }

  size_t size = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(size);
  file.seekg(0);
  file.read(buffer.data(), buffer.size());
  file.close();

  return buffer;
}

} // namespace Utils