//
// Created by spotlight on 1/14/17.
//

#ifndef VULKAN_ENGINE_UTIL_H
#define VULKAN_ENGINE_UTIL_H

#include <vector>
#include <fstream>

namespace engine {
namespace util {

static std::vector<char> readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

}
}

#endif //VULKAN_ENGINE_UTIL_H
