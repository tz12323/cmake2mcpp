#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace mcpp {
class IndexParser {
 public:
  IndexParser() = delete;
  IndexParser(const fs::path& buildDir);
  void ReadIndexFile();
  fs::path Parse();

 private:
  fs::path indexFilePath;
  json indexJson;
};
}  // namespace mcpp
